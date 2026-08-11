# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Submodule of `pinhao`

This repo is also vendored as `mods/araucaria` inside the `pinhao` project and
is commonly edited in place from there. Regardless of which repo you were
invoked from: never commit or push to this repo's own `origin` unless the
user explicitly asks — that's handled by the user separately, same as the
parent's policy for its other submodules.

## Commands

Run from the repo root (`git rev-parse --show-toplevel` from anywhere inside
resolves it — the makefiles depend on this, not on CWD).

| Command | Alias | What it does |
| --- | --- | --- |
| `make build` | `b` | Optimised build (`-O3 -flto`) → `src/main.out` |
| `make dbg` | `d` | Debug build (ASan/UBSan/LeakSanitizer) → `src/debug.out` |
| `make test` | `t` | Build every module in debug mode and run its test suite |
| `make clean` | `c` | Remove objects, binaries and dependency files |
| `make lint` | `l` | Static checks over `lib/*/test/*.c` (requires `python3`) |

`./run.sh` wraps `make build` + `./src/main.out`; `./run_debug.sh` wraps
`make dbg` + `./src/debug.out`. `src/main.c` is a scratch playground of
`[[maybe_unused]]` benchmarks/demos with one enabled in `main` — not the
library's entry point (consumers link `lib/lib.o` directly, see README).

### Running a single test

`make test` at the root runs every module's suite. To iterate on one module:

```bash
make dbg -C lib          # rebuild lib/debug_full.o first (test runners link against it)
make -C lib/num/test      # builds and runs every runner_* in that module
make run_test_1_ram -C lib/num/test   # build+run just one runner
```

Every module has a single `test.c` → `runner_test`, except `num`, which has
three runners sharing case bodies from `behavior.c` and differing only in
allocation strategy: `test_1_ram` (heap only), `test_2_disk`
(`disk_threshold_bytes = 0`, everything on disk), `test_3_mist`
(`disk_threshold_bytes = 8192`, mixed).

Fuzz cases derive their RNG seed from the case tag, so a failure is
reproducible by rerunning the runner binary directly with the seed it
printed on startup:

```bash
SEED=1234567890 ./lib/num/test/runner_test_1_ram
```

Cases run in a forked child under a timeout (`TEST_CASE_TIMEOUT_MS` in
`testrc.h`, 10s); fuzz cases are exempt since large operands can legitimately
take minutes.

`make lint` (`makefiles/lint_tests.py`) catches two things the compiler
can't: duplicate case tags within one test function, and limb fixtures whose
declared count doesn't match the values supplied (which would read
uninitialised stack).

## Architecture

Each module under `lib/` (`num`, `sig`, `fxd`, `flt`, `mod`, `file`) is one
translation unit with a fixed file layout:

- `code.c` — implementation, compiled once into `lib.o` (production) or
  `debug.o` (assertions + sanitizers)
- `header.h` — public API
- `struct.h` — public struct definitions (kept separate from `header.h` so
  consumers can choose opaque vs. transparent access)
- `debug.h` / `internal.h` — internals shared within the module only
- `test/` — its own `Makefile`, linking against the module's `debug_full.o`

The layering is strictly bottom-up: `num` (unsigned) → `sig` (adds sign) →
`fxd`/`flt` (adds fixed/floating point on top of `sig`) → `mod` (modular
arithmetic over a `num`). `file` is a serialisation primitive that `sig`,
`fxd`, `flt` build `*_save`/`*_load` on top of. Don't introduce a dependency
that runs the other direction.

Values are little-endian arrays of 64-bit limbs.

### Multiplication dispatch (`lib/num/code.c`)

`num_mul` picks the algorithm by operand size (threshold `256` limbs,
`code.c:3899`):

- Either operand below the threshold — classic schoolbook.
- Both at or above it — `num_mul_ssm`: Schönhage–Strassen, splitting operands
  into blocks, transforming with a negacyclic FFT modulo `2^(64·(n-1)) + 1`,
  multiplying pointwise, transforming back.

The hot inner loops (classic add/sub/mul, SSM modular add/sub/negate) have
hand-written assembly, selected at compile time via `NUM_ASM_X86_64` (GCC +
BMI2/ADX) or `NUM_ASM_AARCH64`, with a portable C fallback under
`-DNO_ASSEMBLY`. `lib/num` is the hot path of this library — when changing
`code.c`, watch allocation patterns and memory layout, and avoid casual
refactors that add overhead (extra copies, indirection, allocations in inner
loops) in the multiply/asm paths in particular. CI builds and tests both the
assembly and portable paths on both x86-64 and AArch64.

### Disk-backed allocation

`araucaria_disk_config_set` (config struct in `lib/num/struct.h`) redirects
allocations whose backing size exceeds `disk_threshold_bytes` to an
anonymous `mmap` over a temporary file in `disk_path` (unlinked immediately,
so it disappears with the number or the process). Default threshold is
`UINT64_MAX` (nothing goes to disk). This is load-bearing for the `num`/`sig`/`flt` test suites, which
each run their behavior cases three ways (ram/disk/mist) against the same
case bodies — a bug that only reproduces on the disk path is a real category
here, not a hypothetical.

### Ownership

No GC, no refcounting. Operations (`num_add`, `num_mul`, `num_pow`,
`num_div_mod`, etc.) consume and free/reuse their input pointers — the
returned pointer is the only one still valid; pass `num_copy(x)` to keep an
operand alive across a call. Read-only operations (comparisons, predicates,
display) leave arguments untouched. In debug builds `clu` (from
`mods/clu`) tracks every allocation, so `assert(clu_mem_is_empty())` at the
end of a test or program catches a missed `num_free`.

## Compiler flags

`makefiles/flags.mk` enables a large, deliberate warning set (`-Wall
-Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion
-Wnull-dereference -Wcast-qual`, etc.) plus `-fsanitize=address,undefined`
(+`leak` on Linux) in debug builds, with warnings fatal in the real build.
`.clangd` removes `-Werror` for editor diagnostics only — the actual build
still treats warnings as errors. Don't introduce code that only compiles
clean because a warning got suppressed — fix the underlying issue.

## Dependencies

`mods/clu` ([RenanSouza2/clu](https://github.com/RenanSouza2/clu), memory
tracking) and `mods/macros`
([RenanSouza2/c-macros](https://github.com/RenanSouza2/c-macros), shared
assert/test/integer macros) are git submodules, also editable in place — the
same no-push-without-asking rule applies to them.
