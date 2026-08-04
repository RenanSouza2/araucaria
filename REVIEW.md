# Code Review — "Number" arbitrary-precision arithmetic library

Scope: the full `lib/` core (num, sig, mod, fxd, flt) plus `src/main.c`. Every bug
below was verified against the actual source, not just flagged. The `mods/clu` and
`mods/macros` submodules were read only as context.

---

## 1. Overall assessment

**What it is.** A genuinely ambitious, well-engineered bignum library in modern C23.
Numbers are little-endian arrays of 64-bit limbs (`struct num`), and the higher types
compose cleanly on top: `sig` = sign + `num`, `mod` = value + borrowed modulus,
`fxd` = `sig` + fractional-limb position, `flt` = `sig` + limb exponent.

**Engineering maturity is high:**

- **Real algorithms, not toy ones** — Burnikel–Ziegler recursive division, a
  Schönhage–Strassen (FFT) multiplication path, and hand-written x86-64
  `mulx/adcx/adox` assembly kernels, all with portable C fallbacks and both
  exercised in CI.
- **Strong hardening** — `-Werror` with a near-maximal warning set
  (`-Wconversion -Wsign-conversion -Wcast-qual -Wshadow`, plus platform-specific
  `-Wthread-safety -Wconsumed`), ASan+UBSan+LSan in debug builds, `-fanalyzer` in
  CI, and the `clu` allocator that tracks every allocation so tests can
  `assert(clu_mem_is_empty())`.
- **Clean, consistent design** — uniform `module_verb` naming, layered composition,
  a large macro-driven test harness for num/sig/flt, and a Linux/macOS × asm/no-asm
  CI matrix.
- **The core arithmetic is sound.** The consume-ownership model was traced through
  the add/sub/mul/sqr/div paths of sig, mod, fxd, and flt — including the tricky
  `mod_num_div` continued-fraction recursion — and found memory-clean and correct
  against the tests. The bugs below are concentrated in *edge cases, untested
  helpers, serialization, and demo code*, not the hot arithmetic paths.

**The two structural weaknesses that matter most:**

1. **`assert` always calls `exit(EXIT_FAILURE)` — even in release builds**
   (`mods/macros/assert.h`). Every recoverable condition — OOM, malformed input,
   divide-by-zero, I/O failure, `num_unwrap` of a multi-limb value — terminates the
   *host process*. This is the single biggest thing preventing the code from being
   used as an embeddable library; a library should return errors, not kill its
   caller.

2. **Consume-ownership is convention-only.** Read-only ops aren't `const`-qualified,
   so the "this op frees its inputs / this one doesn't" contract lives only in the
   README and the author's head. It's efficient but demonstrably error-prone — the
   leaks in `main.c` (below) are exactly this failure mode.

**Minor.** Build artifacts (`lib/*/lib.o`, `src/main.o`, `src/main.out`) are committed
to git; `src/main.c` is a scratchpad with lots of commented-out dead code; `fxd` is
almost entirely untested (53-line test file vs. num's 3161).

---

## 2. Verified bugs

Severity is calibrated for a **local math library** (no network surface), so "HIGH"
means memory corruption reachable through the public API, not a remote exploit.

### Memory safety

#### ① HIGH — Untrusted `count` in deserialization → integer overflow → heap OOB write
- **Location:** `lib/sig/code.c:403` (`file_read_sig_num_raw`); root cause `lib/num/code.c:376` (`num_create`)
- **Category:** memory-safety / integer-overflow
- **What's wrong:** `count` is read straight from a file, then `num_create(count, count)`
  computes `total_size = sizeof(num_t) + count * 8`. For `count >= 2^61` the multiply
  wraps `uint64_t`, `calloc` returns a ~32-byte buffer, and the loop
  `num->chunk[i] = file_read_uint64(fp)` writes past it — heap corruption from the
  first iteration. Default `disk_threshold = UINT64_MAX`, so the vulnerable `calloc`
  path is the default.
- **Trigger:** `sig_num_load()` on a crafted file with `count = 0x2000000000000000`
  followed by a few hundred 8-byte words. Only relevant if untrusted files are
  deserialized, but it's a clean overflow. The same read-length-then-allocate pattern
  appears in the flt/fxd file loaders — audit them the same way.
- **Fix:** reject `count > (remaining_file_bytes / 8)` before `num_create`; add an
  overflow check (`__builtin_mul_overflow`) inside `num_create`.

#### ② MEDIUM — `num_sub` corrupts the heap when the subtrahend is larger
- **Location:** `lib/num/code.c:3220` (`num_sub`) → `lib/num/code.c:956` (`num_sub_offset`)
- **Category:** memory-safety
- **What's wrong:** `num_sub` calls `num_sub_offset(num_1, 0, num_2)` with no size
  guard. The loop writes `num_1->chunk[i]` for `i < num_2->count`; if
  `num_2->count > num_1->size` it writes out of bounds. The `assert(borrow == 0)`
  meant to catch the underflow runs only *after* the whole loop — too late. Contrast
  `num_add` (line 3213), which correctly does `num_expand_to(num_1, count + 1)` first.
- **Trigger:** `num_sub(num_wrap(1), two_limb_num)` — unsigned `a - b` with `b > a`.
  A precondition violation, but the API corrupts memory instead of failing cleanly.
- **Fix:** `assert(num_cmp(num_1, num_2) >= 0)` at the top of `num_sub`, before any write.

#### ③ MEDIUM — `num_create_disk` never checks the `mmap` result
- **Location:** `lib/num/code.c:359` (`num_create_disk`)
- **Category:** memory-safety / robustness
- **What's wrong:** `num_p num = mmap(...); close(fd); *num = (num_t){...};` — every
  other syscall here is asserted (`fd`, `ftruncate`) except `mmap`. On failure `mmap`
  returns `MAP_FAILED` (`(void*)-1`) and the very next line writes through it → SIGSEGV.
- **Trigger:** any disk-backed allocation under address-space pressure / `RLIMIT_AS`
  (`test_2_disk` sets `disk_threshold = 0`, so every number mmaps).
- **Fix:** `assert(num != MAP_FAILED);` after the call. Also guard `total_size`
  (line 357) against multiplication overflow.

### Memory leaks

#### ④ HIGH — the one active demo leaks and aborts the debug build
- **Location:** `src/main.c:712` (`time_assembly_benchmark`, the sole function `main()` runs)
- **Category:** memory-leak
- **What's wrong:** `num_1` is created, used only via `num_copy` (read-only), and never
  freed. `num_2`/`num_1_c` are consumed by `num_add`/`num_mul`; `num_res` is freed —
  but `num_1` is not. Under `run_debug.sh` the `assert(clu_mem_is_empty())` at line 737
  fails and aborts; in release it silently leaks a ~GB-scale number until exit.
- **Trigger:** running the program (this is what `main()` at `src/main.c:770` invokes).
- **Fix:** `num_free(num_1);` before the `#ifdef DEBUG` block (after line 731).

#### ⑤ MEDIUM — `fxd_num_base_to` leaks on every call
- **Location:** `lib/fxd/code.c:298` (`fxd_num_base_to`)
- **Category:** memory-leak
- **What's wrong:** the `while(true)` loop breaks (line 288) with `num_lo` and `num_u`
  still holding live allocations; only `num_hi` is stored back into `fxd.sig.num`.
  Both leak on every call (untested public API).
- **Trigger:** any call to `fxd_num_base_to`.
- **Fix:** `num_free(num_lo); num_free(num_u);` before `fxd.sig.num = num_hi;`.

#### ⑥ MEDIUM — `flt_num_display_dec` leaks `flt_1` (always) and the rescaled `flt_0`
- **Location:** `lib/flt/code.c:175` and `lib/flt/code.c:254` (`flt_num_display_dec`)
- **Category:** memory-leak
- **What's wrong:** `flt_1 = flt_num_copy(flt_0)` (line 175) is used throughout but
  never freed. In the `if(base != 0)` branch, `flt_0` is reassigned to a new allocation
  (line 254) whose `sig.num` is wrapped into `fxd` and never reclaimed.
- **Trigger:** any call to `flt_num_display_dec` leaks `flt_1`; calls needing base
  scaling leak an additional `flt`.
- **Fix:** free `flt_1` before returning; free the intermediate `flt_0` after use.

#### ⑦ LOW–MEDIUM — several other `main.c` leaks (driver code)
- **Location:** `src/main.c:432` (`pi_1`), plus `time_2` (110,117), `time_3` (164),
  `time_dec` (401), `fibonacci_2/3` (243,277), `sqrt_2` (629)
- **Category:** memory-leak
- **What's wrong:** `pi_1` leaks one `fxd` *per iteration* — the `fxd_num_free(fxd_1)`
  is wrongly nested inside the `i % 1000000 == 0` branch (unbounded growth). The others
  never free their accumulators before returning. No use-after-consume or double-free
  anywhere in `main.c` — purely leaks.
- **Trigger:** running the respective demo functions.
- **Fix:** move `pi_1`'s free out of the `if`; free accumulators after each loop.

### Correctness

#### ⑧ MEDIUM — `sig_num_mul_int(x, 0)` breaks the zero-signal invariant
- **Location:** `lib/sig/code.c:567` (`sig_num_mul_int`)
- **Category:** correctness
- **What's wrong:** multiplying by 0 yields a zero magnitude but returns the *original*
  `POSITIVE`/`NEGATIVE` signal (it doesn't route through `sig_num_create`, which
  re-normalizes zero to `ZERO`).
- **Trigger:** `sig_num_is_zero(sig_num_mul_int(sig_num_wrap(5), 0))` returns `false`;
  anything relying on "zero ⇒ signal == ZERO" misbehaves.
- **Fix:** `return sig_num_create(sig.signal, num_mul_uint(sig.num, (uint64_t)value));`

#### ⑨ LOW — `fxd_num_base_to` drops a limb when the integer part is zero
- **Location:** `lib/fxd/code.c:293` (`fxd_num_base_to`)
- **Category:** correctness
- **What's wrong:** `num_head_grow(num_hi, 1)` is a no-op when `num_hi->count == 0`
  (see `lib/num/code.c:464`), so the following `num_hi->chunk[0] = ...` writes a limb
  that `count` (still 0) does not cover; the digit is silently lost.
- **Trigger:** converting a purely-fractional fixed-point value (integer part zero) to
  another base yields a wrong result.
- **Fix:** don't rely on `num_head_grow` for a zero num; set count explicitly or
  special-case the zero-`num_hi` path.

### Undefined behavior / integer overflow

#### ⑩ MEDIUM — `int64_add` computes the sum *before* the overflow check → UB the optimizer can delete
- **Location:** `lib/flt/code.c:284` (`int64_add`)
- **Category:** UB / integer-overflow
- **What's wrong:**
  ```c
  int64_t res = a + b;                          // signed overflow = UB, happens first
  if (a && b && sign_a == sign_b)
      assert(sign_a == int64_get_sign(res));    // the guard
  ```
  Release is `-O3 -march=native -flto` with no `-fwrapv` (`makefiles/flags.mk`), so the
  compiler may assume no signed overflow, prove the assert always true, and remove the
  exact overflow guard the tests check — letting `flt` exponents silently wrap.
- **Trigger:** in a release build, `flt_num_mul`/`sqr`/`add` on operands whose exponents
  sum past `INT64_MAX`.
- **Fix:** `assert(!__builtin_add_overflow(a, b, &res));` (the `num` layer already uses
  `__builtin_*_overflow` correctly).

#### ⑪ LOW — `INT64_MIN` negation / subtraction UB at the extremes
- **Location:** `lib/sig/code.c:564` (`sig_num_mul_int`), `lib/flt/code.c:635`
  (`flt_num_pow`), `lib/flt/code.c:593` (`flt_num_add`)
- **Category:** UB / integer-overflow
- **What's wrong:** `value = -value` is UB when `value == INT64_MIN`; `flt_num_add` does
  raw `exponent - 1`, which underflows at `INT64_MIN`. Works by luck on two's-complement,
  trips `-fsanitize=undefined` in debug.
- **Trigger:** passing `INT64_MIN` to `sig_num_mul_int`/`flt_num_pow`; adding two flts
  both at `exponent == INT64_MIN`.
- **Fix:** compute magnitudes in unsigned/128-bit space; use `int64_sub` instead of raw `-`.

### Quality nits worth fixing
- `num_config.disk_path` defaults to `NULL` → `snprintf("%s/...", NULL)` is UB if a
  caller sets `disk_threshold` but not `disk_path` (`lib/num/code.c:352`).
- Dead preprocessor branch: `lib/num/code.c:1811` uses `#elif` with a condition
  *identical* to its `#if`, so the Apple/ARM assembly in `num_ssm_sub_mod_immed` is
  unreachable (siblings correctly test `__APPLE__`). Harmless today (C fallback runs)
  but a latent trap.
- `int_rand(min, max)` divides by zero when `min == max` (debug-only, `lib/flt/code.c:26`).

---

## 3. Bottom line

This is strong, sophisticated code with excellent tooling discipline, and its core
arithmetic held up under scrutiny. The defects cluster in three predictable places:
**untested helpers** (`fxd_num_base_to`, `sig_num_mul_int`), **serialization of
untrusted input** (①), and **the demo driver** (`main.c`).

Two things to prioritize regardless of the individual bugs:

1. Reconsider **assert-always-`exit`** if this is ever meant to be embedded — a library
   should not terminate its host process on recoverable errors.
2. Add a **bounds/overflow check to `num_create`** — this fixes ① and hardens ③ at the
   root.

The SSM/FFT and Burnikel–Ziegler paths were confirmed memory-safe, but their deep
*numeric* correctness was leaned on the fuzz tests rather than proven by hand — a
worthwhile target for follow-up verification.
