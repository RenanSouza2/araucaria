# Stall kit

Tools used to find where a disk-backed multiply spends wall time while neither
the CPU nor the disk is busy, plus two optional patches. Everything was written
and run on macOS. The Linux parts read `/proc` and compile here, but have
**never been run on Linux**.

`WSL.md` is the plan for continuing this on the WSL box: start there.

Nothing in this directory is built by the repo's makefiles.

| File | What it is |
| --- | --- |
| `WSL.md` | The runs still owed on the WSL box, step by step. |
| `diskbw.c` | Raw file throughput in the shapes the staged transform uses. Standalone. |
| `probe-harness.patch` | Adds `src/probe.c`, a `probe` entry in `src/main.c` and `lib/num/prof.h`. Changes no library file. |
| `probe-hooks.patch` | The `PROF_` hooks for `lib/num/code.c` as it stands on this branch. |
| `probe-hooks-baseline.patch` | The same hooks for `lib/num/code.c` as it was at `ae5e242`, to measure the old code. |
| `analyze.py` | Turns one probe run into a per-phase table. |
| `check.py` | Runs a probe binary over small sizes, thread counts and budgets and checks every product. |
| `run.sh` | One probed multiply, with its report saved next to its logs. |
| `split-all-workers.patch` | Optional. Lets every worker take part in the split phase of an in-RAM transform. |
| `direct-io.patch` | Optional, experimental. Moves staged transform arrays past the page cache. No gain on macOS. |
| `probe-hooks-direct-io.patch` | The hooks for `lib/num/code.c` with `direct-io.patch` applied. They leave out the depad rounds, and `PROBE_IOVERIFY` checks nothing there. |

The harness needs one of the hook patches to measure anything, and none of the
patches is meant to stay applied. `probe-hooks.patch`, `split-all-workers.patch`
and `direct-io.patch` are each cut against this branch's `lib/num/code.c` and do
not stack on one another.

## 1. Disk shapes (5 minutes, no araucaria needed)

```bash
cc -O2 -o diskbw diskbw.c -pthread
D=/path/on/the/scratch/volume        # pinhao's cache/swap
# file MB, element KB, threads, pattern, flags, ops
./diskbw $D 32768 480 1  seq  -  ww     # one writer: new blocks, then a rewrite
./diskbw $D 32768 480 16 seq  -  ww     # 16 writers, buffered
./diskbw $D 32768 480 16 wide -  wxx    # strided; x = half read while half write
./diskbw $D 32768 480 16 wide p  wxx    # same, file preallocated
./diskbw $D 32768 480 16 wide s  wxx    # same, writeback started per element (Linux)
./diskbw $D 32768 480 16 wide n  wxx    # same, O_DIRECT
./diskbw $D 32768 480 16 wide np wxx    # O_DIRECT, preallocated
./diskbw $D 32768 16384 16 seq - wxx    # 16 MB transfers, buffered
```

Use a file of at least 1.5x RAM. `480` KB is one transform element at 4G-limb
operands, and `wide` is the access pattern of a strided pass.

Read it as: the best row is what the volume can do in that shape. A row far
below it with `in-call` near the thread count is time lost inside the kernel
rather than at the device, and `sys` is the kernel CPU that pass burned. A first
`w` much slower than the second means allocating new blocks is the slow part.

## 2. One instrumented multiply

```bash
git apply stall-kit/probe-harness.patch stall-kit/probe-hooks.patch
make clean && make build FLAGS_EXTRA="-DARAUCARIA_PROF -DASSERT_VERBOSE"
# name, probe binary, limbs per operand, threshold MB, budget MB
D=$D L=/tmp/run1 stall-kit/run.sh big ./src/main.out 1073741824 12800 1600
git apply -R stall-kit/probe-hooks.patch stall-kit/probe-harness.patch && make clean
```

`run.sh` starts the probe with every thread of the machine and leaves
`/tmp/run1/big.report.txt` next to the probe's own logs. By hand it is:

```bash
# log prefix, limbs per operand, threads, threshold MB, budget MB, seed, mul|sqr
PROBE_DISK_PATH=$D ./src/main.out probe /tmp/run1/big 1073741824 16 12800 1600 1 mul \
    | tee /tmp/run1/big.stdout.txt
python3 stall-kit/analyze.py /tmp/run1/big --rounds
```

`threshold` and `budget` are `disk_threshold_bytes` and `ram_budget_bytes` in
MB. pinhao sets them to `mem_max / 2` and `mem_max / n_process`. Without
`PROBE_DISK_PATH` the scratch files go to `./cache`, which must exist.

If GCC rejects something in `probe.c`, add `-Wno-error` to `FLAGS_EXTRA`: the
file has only been through clang.

The probe multiplies two random operands and checks the product's residue mod
2^64 - 59 against the product of the operands' residues, so every run is also a
correctness check. `check.py <binary> quick|wide|full mul|sqr` does that over a
matrix of small sizes in a few minutes; `wide` is 12 to 32 threads.

Environment switches, each on when set to anything:

- `PROBE_IOVERIFY` keeps a hash of every element the staged transform writes
  and aborts with `IOVERIFY` if one reads back differently, saying how many of
  its limbs are zero. It separates a storage stack that loses writes from
  arithmetic that is wrong.
- `PROBE_SLOW_MS=2000` makes one worker per pass sleep that long per block, and
  `PROBE_STATIC` gives staged pass workers fixed ranges again. Together they
  show what one slow worker costs.
- `PROBE_DIRECT`, in a build with `direct-io.patch` and
  `-DPROBE_HAS_DIRECT_IO`, turns direct I/O on.

## 3. Reading the output

`<prefix>.samples.tsv` has one row per second:

- `cpu`: cores the process is using. `in_rd` / `in_wr`: threads inside
  `pread` / `pwrite`. A thread in neither and not on a CPU is blocked on a
  page fault, a lock or a join.
- `dev_rd_mb`, `dev_wr_mb`, `dev_util`, `dev_queue` (Linux): whole disks from
  `/proc/diskstats`.
- `iowait`, `dirty_mb`, `wback_mb`, `psi_io_*`, `psi_mem_*`, `allocstall_s`,
  `scan_direct_s` (Linux): dirty-page throttling and reclaim. `psi_mem_full`
  above zero means threads stalled in reclaim.

`analyze.py` takes the log prefix and prints, per phase: wall time, cores busy,
threads in I/O calls, threads idle and device throughput. Then the share of the
threads' time spent on a CPU, inside `pread`/`pwrite`, and in neither. It reads
the thread count and the result lines from `<prefix>.stdout.txt` when the
probe's output was saved there; otherwise pass `--threads`.
