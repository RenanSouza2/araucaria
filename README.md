# Araucaria

An arbitrary precision arithmetic library written in C23. It provides unsigned
and signed integers, fixed-point, floating-point and modular numbers on top of
a single limb-based core.

What sets it apart:

- **Schönhage–Strassen multiplication** for large operands, with hand-written
  x86-64 (BMI2/ADX) and AArch64 assembly inner loops — not just a naive
  bignum implementation.
- **Disk-backed numbers.** Past a configurable size, values live in
  `mmap`-ed temporary files instead of the heap, so a single computation can
  outgrow RAM.
- **Explicit, predictable ownership.** No GC, no refcounting — operations
  consume their inputs and return the only valid pointer, which keeps
  performance predictable in hot loops.

## Getting started

Araucaria is meant to be vendored as a git submodule and linked into your own
project — `src/main.c` in this repo is just a scratch playground for
benchmarks and demos, not the library's entry point.

```bash
git submodule add https://github.com/RenanSouza2/araucaria.git mods/araucaria
git submodule update --init --recursive   # pulls in araucaria's own clu/macros submodules
```

Build the library object and link it into your binary:

```bash
make build -C mods/araucaria/lib   # -> mods/araucaria/lib/lib.o
```

Then include the header file:

```c
#include "mods/araucaria/header.h"
```

Requires `gcc` with C23 support and a POSIX platform (Linux and macOS are
both exercised in CI); the build uses `-march=native`.

## Usage example

```c
#include "mods/araucaria/lib/num/header.h"

int main()
{
    num_p a = num_wrap(2);      // 2
    num_p b = num_pow(a, 100);  // consumes a; b = 2^100

    num_display_dec(b);         // does not consume b

    num_free(b);
    return 0;
}
```

From a consuming project that's `mods/araucaria/lib/lib.o` (or
`mods/araucaria/lib/debug_full.o`, built with `make dbg`, which also pulls in
the `clu` allocation tracker) — see [Getting started](#getting-started).

## Modules

| Module | Type | Summary |
| --- | --- | --- |
| `num` | `num_p` | Arbitrary precision **unsigned integers**. The core. Add, sub, mul, div/mod, pow, square, gcd, shifts, base conversion, decimal I/O, disk backing. |
| `sig` | `sig_num_t` | **Signed integers** — a `num` plus a sign. |
| `fxd` | `fxd_num_t` | **Fixed-point** — a `sig` plus a fixed limb position, for high-precision fractions. |
| `flt` | `flt_num_t` | **Floating-point** — a `sig` with an exponent and a target mantissa size. Includes `flt_num_safe_add`, which reports when a term no longer affects the result (useful as a series termination test). |
| `mod` | `mod_num_t` | **Modular arithmetic** over a `num` modulus. |
| `file` | `file_p` | Binary serialisation primitives; `sig`, `fxd` and `flt` build `*_save` / `*_load` on top of it. |

Values are stored little-endian as arrays of 64-bit limbs.

## Disk-backed numbers

```c
#include "mods/araucaria/lib/num/header.h"
#include "mods/araucaria/lib/num/struct.h"

araucaria_disk_config_t config = {
    .disk_path      = "./cache",  // must already exist
    .disk_threshold = 1024,       // limbs; larger allocations go to disk
};
araucaria_disk_config_set(&config);
```

Allocations above `disk_threshold` limbs come from an anonymous `mmap` over a
temporary file that's `unlink`-ed immediately, so it disappears when the
number is freed or the process exits. The default threshold is `UINT64_MAX`
(nothing goes to disk); `num_realloc_disk` (and the `sig` / `flt`
equivalents) moves an already-allocated value to disk on demand.

## Ownership

- **Operations consume their inputs.** `num_add`, `num_mul`, `num_pow`,
  `num_div_mod` and friends free or reuse the pointers passed to them; the
  returned pointer is the only valid one afterwards. To keep an operand
  alive, pass `num_copy(x)` instead of `x`.
- **Read-only operations do not.** Comparisons, predicates and display
  functions leave their arguments untouched.

In debug builds, `clu` tracks every allocation so `assert(clu_mem_is_empty())`
at the end of a program catches a missed `num_free`.

`mods/clu` is a separate memory-debugging library
([RenanSouza2/clu](https://github.com/RenanSouza2/clu)); `mods/macros` holds
the shared assert/test/integer macros
([RenanSouza2/c-macros](https://github.com/RenanSouza2/c-macros)).

## Development

| Target | Alias | What it does |
| --- | --- | --- |
| `make build` | `b` | Optimised build (`-O3 -flto`) → `src/main.out` |
| `make dbg` | `d` | Debug build (ASan/UBSan/LeakSanitizer) → `src/debug.out` |
| `make test` | `t` | Build every module in debug mode and run its test suite |
| `make clean` | `c` | Remove objects, binaries and dependency files |
| `make lint` | `l` | Static checks over the test sources (requires `python3`) |

Each module under `lib/` has its own `test/` directory; see the module for
details. CI (`.github/workflows/test.yml`) builds and tests on Linux and
macOS, with and without the assembly inner loops.
