# Araucaria

An arbitrary precision arithmetic library written in C23. It provides unsigned
and signed integers, fixed-point, floating-point and modular numbers on top of a
single limb-based core, with a Schönhage–Strassen multiplier and hand-written
x86-64 / AArch64 assembly inner loops.

Numbers larger than a configurable threshold can be backed by `mmap`-ed
temporary files instead of the heap, so a computation can outgrow RAM.

## Requirements

- `gcc` with C23 support (`-std=c23`). The build uses `-march=native`, so
  binaries are tuned for the machine that compiled them.
- A POSIX platform. Linux and macOS are both exercised in CI.
- `python3`, only for `make lint`.
- The `clu` and `macros` submodules (see below).

## Getting started

```bash
git clone --recurse-submodules https://github.com/RenanSouza2/araucaria.git
cd araucaria

# if you already cloned without --recurse-submodules
git submodule update --init --recursive

make          # build src/main.out
./run.sh      # rebuild and run it
```

`src/main.c` is a scratch driver: a collection of `[[maybe_unused]]` benchmarks
and demos (multiplication timings, Fibonacci, factorial, π and *e* series,
`sqrt(2)` by Newton iteration) with one of them enabled in `main`. Edit it to
pick what runs.

### Make targets

Run from the repo root. Each has a one-letter alias.

| Target | Alias | What it does |
| --- | --- | --- |
| `make build` | `b` | Optimised build (`-O3 -flto`) → `src/main.out` |
| `make dbg` | `d` | Debug build → `src/debug.out` |
| `make test` | `t` | Build every module in debug mode and run its test suite |
| `make clean` | `c` | Remove objects, binaries and dependency files |
| `make lint` | `l` | Static checks over the test sources |

`./run.sh` wraps `make build` + `./src/main.out`; `./run_debug.sh` wraps
`make dbg` + `./src/debug.out`. Both forward their arguments and clear
`thread_log/` first.

Debug builds add `-D DEBUG -O0 -g3`, ASan and UBSan (plus LeakSanitizer on
Linux), enable the assertions throughout the library, and link `clu` for
allocation tracking.

## Modules

Each module under `lib/` is one translation unit (`code.c`) plus a public
`header.h`, an internal `debug.h` and a `struct.h`.

| Module | Type | Summary |
| --- | --- | --- |
| `num` | `num_p` | Arbitrary precision **unsigned integers**. The core. Add, sub, mul, div/mod, pow, square, gcd, shifts, base conversion, decimal I/O, disk backing. |
| `sig` | `sig_num_t` | **Signed integers** — a `num` plus a sign. |
| `fxd` | `fxd_num_t` | **Fixed-point** — a `sig` plus a fixed limb position, for high-precision fractions. |
| `flt` | `flt_num_t` | **Floating-point** — a `sig` with an exponent and a target mantissa size. Includes `flt_num_safe_add`, which reports when a term no longer affects the result (useful as a series termination test). |
| `mod` | `mod_num_t` | **Modular arithmetic** over a `num` modulus. |
| `file` | `file_p` | Binary serialisation primitives; `sig`, `fxd` and `flt` build `*_save` / `*_load` on top of it. |

Values are stored little-endian as arrays of 64-bit limbs (`chunk_bits = 64`).

## Multiplication

`num_mul` dispatches on operand size:

- **Either operand below 256 limbs** — classic schoolbook multiplication.
- **Both at 256 limbs or more** — `num_mul_ssm`, a Schönhage–Strassen
  multiplier: the operands are split into `K` blocks of `M` limbs, transformed
  with a negacyclic FFT modulo `2^(64·(n-1)) + 1`, multiplied pointwise and
  transformed back.

The hot inner loops (classic add/sub/mul, and the SSM modular add/sub/negate)
have hand-written assembly variants, selected at compile time:

- `NUM_ASM_X86_64` — requires GCC (not clang) with BMI2 and ADX.
- `NUM_ASM_AARCH64` — any AArch64 target.

Compile with `-DNO_ASSEMBLY` to force the portable C path. CI builds and tests
both ways on both platforms.

## Disk-backed numbers

By default every `num` is heap allocated. Setting a disk configuration makes
allocations above `disk_threshold` limbs come from an anonymous `mmap` over a
temporary file in `disk_path` instead, letting a single value exceed available
RAM.

```c
#include "lib/num/header.h"
#include "lib/num/struct.h"

araucaria_disk_config_t config = {
    .disk_path      = "./cache",  // must already exist
    .disk_threshold = 1024,       // limbs; larger allocations go to disk
};
araucaria_disk_config_set(&config);
```

The backing file is `unlink`-ed immediately after creation, so it disappears
when the number is freed or the process exits. `num_realloc_disk` (and the
`sig` / `flt` equivalents) moves an already-allocated value to disk on demand.
A threshold of `0` sends everything to disk; the default is `UINT64_MAX`, which
sends nothing.

## Memory management & ownership

The library has no garbage collection and no reference counting. Ownership is
explicit and follows two rules:

- **Operations consume their inputs.** `num_add`, `num_mul`, `num_pow`,
  `num_div_mod` and friends take ownership of the pointers passed to them and
  will free or reuse their storage. The returned pointer is the only valid one
  afterwards. To keep an operand alive, pass `num_copy(x)` instead of `x`.
- **Read-only operations do not.** Comparisons (`num_cmp`), predicates
  (`num_is_zero`) and the display functions leave their arguments untouched;
  you still own them.

In debug builds `clu` tracks every allocation, so `assert(clu_mem_is_empty())`
at the end of a program catches a missed `num_free`. `clu_get_max_occupancy()`
reports peak usage — `src/main.c` uses both.

## Usage example

```c
#include "lib/num/header.h"

int main()
{
    num_p a = num_wrap(2);      // 2
    num_p b = num_pow(a, 100);  // consumes a; b = 2^100

    num_display_dec(b);         // does not consume b

    num_free(b);
    return 0;
}
```

Link against `lib/lib.o` (or `lib/debug_full.o`, which also pulls in `clu`);
`src/Makefile` shows the flags.

## Testing

```bash
make test          # every module
make -C lib/num test
```

Each module has a `test/` directory with its own runner. `num` has three,
sharing the case bodies in `behavior.c` and differing only in allocation
strategy:

| Runner | Threshold | Coverage |
| --- | --- | --- |
| `test_1_ram.c` | unset | pure heap |
| `test_2_disk.c` | `0` | every allocation on disk |
| `test_3_mist.c` | `1024` | mixed heap and disk |

Cases run in a forked child with a timeout (`TEST_CASE_TIMEOUT_MS` in
`testrc.h`, 10s; fuzz cases are exempt because large operands legitimately take
minutes). Fuzz cases derive their RNG seed from the case tag, so a failure is
reproducible:

```bash
SEED=1234567890 ./lib/num/test/runner_test_1_ram
```

The runner prints the seed it used at startup.

`make lint` runs `makefiles/lint_tests.py` over `lib/*/test/*.c` and flags two
things the compiler cannot: duplicate case tags within one test function (which
would make a failure report ambiguous) and limb fixtures whose declared count
does not match the values supplied (which would read uninitialised stack).

## Layout

```
lib/            core modules — num, sig, fxd, flt, mod, file
src/            main.c driver and benchmarks
mods/           submodules: clu (memory tracking), macros (assert, test, uint, time)
makefiles/      shared make fragments and the test linter
  flags.mk        compiler/linker flags, per-platform
  vars.mk         project paths
  lib.mk          per-module build
  bundle.mk       aggregation of module objects
  test.mk         single-runner test build
header.h        umbrella header
testrc.h        test configuration
```

`mods/clu` is a separate memory-debugging library
([RenanSouza2/clu](https://github.com/RenanSouza2/clu)); `mods/macros` holds the
shared assert/test/integer macros
([RenanSouza2/c-macros](https://github.com/RenanSouza2/c-macros)).

## CI

`.github/workflows/test.yml` runs a matrix on every push:
`{ubuntu-26.04, macos-latest} × {build, test} × {default, no_assembly}`, plus
`make lint`. Linux builds additionally enable GCC's `-fanalyzer`.
