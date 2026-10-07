# Handoff: what to run on the WSL box

Written for whoever continues this work on the WSL machine with none of the
earlier conversation. Read it through once before running anything.

## The problem

Very large multiplications in pinhao stall on the WSL box: for long stretches
neither the disk nor the CPU is the bottleneck. The user asked for the cause to
be found and then fixed in `lib/num/code.c`, on these terms:

- diagnose first, then fix only what the evidence supports;
- measure each fix before and after at large size, and check every product;
- leave changes uncommitted for their review unless they say otherwise;
- tell them if a power-saving mode ever turns on during a run. It should not.

Everything so far was done on a MacBook Air M2 (8 cores, 8 GB RAM, APFS on the
internal NVMe), which reproduces stalls of its own but is not the machine the
complaint is about. **This branch has never run on this box, on WSL, or on
x86-64 beyond the size of a unit test.** Its only contact with Linux is CI,
which builds it with GCC and passes the test suite on Ubuntu x86-64 (see step
1). Of its code, only the three commits it brings back have run on this box, in
September.

## What is on this branch

`stall-followup` is `omg` at `ae5e242` plus nine commits: the seven of
`stall-fixes`, then a test and this file's update. Each message carries its own
measurements:

```bash
git log --reverse ae5e242..stall-followup
```

| # | Commit | Why |
| --- | --- | --- |
| 1 | Brings back c136164, d457e63 and 5d12804, which c20d934 reverted | The depad ran through the mapping with 7 of 8 threads parked on page faults; each transform swept the array three times where two fit |
| 2 | The pad reads a disk-backed operand with explicit reads | It faulted the operand in through the mapping |
| 3 | One `pread` or `pwrite` call moves at most 1 GiB | macOS fails a request past `INT_MAX` |
| 4 | Squaring's pointwise step runs inside the first inverse pass | It ran through the mapping, and wrote the array once more |
| 5 | Staged pass workers take blocks off a shared counter | With fixed shares one slow worker idled the rest |
| 6 | The squaring kernel zeroes only the limbs it writes | It zeroed the whole scratch buffer on every call |
| 7 | This directory | |
| 8 | Products at 12 and 16 threads in the stage test runner | Nothing in `make test` staged a transform with more than 6 threads |
| 9 | This file brought up to date | |

Mac results, 8 threads, threshold 1024 MB, budget 2048 MB, AC power, low power
mode off, every product checked. Operands of 2^28, 2^30 and 3 x 2^30 limbs:

| Test | `ae5e242` | this branch |
| --- | --- | --- |
| 2 GB x 2 GB | 162 s | 100-103 s |
| 8 GB x 8 GB | 751 s | 457 s |
| 24 GB x 24 GB | 3331 s | 1957 s |
| 8 GB squared | stopped after 21 minutes, pointwise step 39% done | 301 s |
| 24 GB squared | not run | 1197 s |

Share of the threads' time spent neither on a CPU nor inside `pread`/`pwrite`:
14%, 21% and 20% before, 1% after. What is left on the Mac is the device itself
(strided passes at 0.7-1.0 GB/s each way with every thread inside a read or a
write) and one CPU-bound pass, the pointwise multiply.

Two commits whose messages say they were not timed apart were timed later, at
8 GB, one run each, back to back. The machine was warm by then (the tip squared
8 GB in 346 s, against 301 s in the table), so these compare with each other
only:

| Commit | Without it | With it | Page-ins |
| --- | --- | --- | --- |
| 2, the pad read (8 GB x 8 GB) | 519 s | 516 s | 1,058,250 to 265 |
| 4, the fused square (8 GB squared, commit 6 in both) | 676 s | 346 s | 2,130,087 to 128 |

So the pad read does not change the total on the Mac; it only stops the operand
coming in by page faults. A likely reason, not checked: that pass waits on the
device whichever way the operand is read.

## What is not known

In order of how much it matters:

1. **Whether products come out right on this box.** Commit 1 brings back the
   code c20d934 dropped. That revert followed runs on this box (then 16 GB RAM,
   threshold 5120 MB, budget 1280 MB, 16 threads, 4G x 4G limbs) where the
   product was wrong in one of two runs of c136164 and one of two of d042c8e,
   and right in the single runs of eef0990 and af08b71. The cause was never
   found. 37faca6 describes the damage: one corrupted transform element reaches
   every limb of the product, and `num_mul_ssm` now asserts the product's length
   so that most such products abort instead of being returned. pinhao runs have
   shown the same damage.
   On the Mac no wrong product could be produced: about 2,300 checked products
   at 1 to 8 threads up to 24 GB operands, about 1,750 more at 12, 16, 24 and
   32 threads spread over every commit of the series and the old code, and
   ThreadSanitizer reports nothing at 1 to 32 threads. So either the fault
   needs this machine (x86-64, Linux, this storage stack) or it is rare. A
   product that fails here is the most useful thing this box can produce: see
   *A wrong product* below.
2. **Whether the test suite passes with this box's toolchain.** It does in CI:
   Ubuntu 26.04, GCC with this repo's Linux-only warnings and `-fanalyzer`,
   with the x86-64 assembly and without. That includes commit 6 through the
   assembly of `num_sqr_classic_buffer`, which the Mac could only read. So step
   1 should pass, and a failure there would be about this box.
3. **Whether the staged path is right at 16 threads on this box.** Until commit
   8 nothing in `make test` staged a transform with more than 6 threads, and
   pinhao runs 16. Commit 8 adds products at 12 and 16 threads to the stage
   runner, 131K to 400K limbs, and CI passes them on x86-64. *Small products
   first* in step 3 goes to 30M limbs and more budgets.
4. **Whether any of it is faster here.** In the same September runs the three
   commits were no faster on this box: 5780 s for af08b71, 6143-6579 s with
   them, and the depad took 345-460 s staged or not.

## Ground rules

- Do the steps in order. Step 1 gates the rest.
- Ask the user once before the `b_` runs of step 3: each takes hours and 275G
  of scratch.
- Run nothing else heavy on the box meanwhile, pinhao included.
- If the host is a laptop, keep it on AC and check Windows is not in battery
  saver or a power-saving plan (`powercfg.exe /getactivescheme` works from
  inside WSL). Tell the user if that changes during a run.
- Do not commit or push without asking. Never push to `omg`.
- Keep every log, outside the repo.
- A failed build, a failed test, an assertion or a `VERIFY FAIL` is a result,
  not an obstacle: stop, keep the output, report it.
- Two numbers that should match and differ by more than a tenth: run both again
  before reading anything into it.

## Setup

Work in a clone of its own, so pinhao's `mods/araucaria` stays as it is:

```bash
git clone -b stall-followup --recurse-submodules \
    https://github.com/RenanSouza2/araucaria.git ~/araucaria-stall
cd ~/araucaria-stall
cat > ~/stall-env.sh <<'END'
export D=/path/to/pinhao/cache/swap     # the scratch directory pinhao uses here
export L=~/stall-logs                   # every log goes here
export T=12800 B=1600                   # pinhao's settings in MB, see below
END
. ~/stall-env.sh && mkdir -p $L
```

Every command below is run from `~/araucaria-stall` with those four variables
set. A shell that does not keep variables from one command to the next needs
`. ~/stall-env.sh` in front of each.

`T` and `B` are `disk_threshold_bytes` and `ram_budget_bytes` in MB (2^20
bytes). pinhao sets them in `pi()` in its `src/main.c` to `mem_max / 2` and
`mem_max / n_process`. The committed `mem_max` of 25 GiB with 16 processes gives
12800 and 1600; use what the checkout on this box actually has.

## Step 0: record the machine

```bash
uname -a; nproc; free -g; gcc --version | head -1
df -hT $D; findmnt -T $D
sysctl vm.dirty_ratio vm.dirty_background_ratio vm.dirty_bytes vm.swappiness
ls /proc/pressure
```

From Windows, also note the contents of `.wslconfig` (memory, swap, and any
`sparseVhd`, `autoMemoryReclaim` or `pageReporting` line), `wsl --version`, the
size of the distro's `ext4.vhdx`, and the free space on the drive that holds
it. A host drive with less free space than a run's scratch can lose writes the
guest has already been told succeeded.

## Step 1: build and test

```bash
make clean && make build && make dbg && make test && make lint
```

All of it must pass. `make test` takes 15 minutes on the Mac and 5 to 10 in
CI, which runs these same targets on every push (`.github/workflows/test.yml`):
on Ubuntu x86-64 with GCC, the first eight commits of this branch build and
pass, assembly and portable. CI's macOS test jobs run into their 45-minute
limit, on `omg` as well; that is not a failure of the branch.

- A GCC warning that stops the build means this box's GCC differs from CI's:
  note both versions, fix it in the smallest way, keep the change as a diff to
  report, carry on.
- A failing test: note the runner (`ram`, `disk`, `mist`, `stage`), the case and
  the seed it printed, then stop. A squaring case points at commit 6.
- Every commit builds and passes `make test` on the Mac, so a bisect is
  meaningful: `git bisect start stall-followup ae5e242`, then
  `git bisect run make test -C lib/num`. The 12- and 16-thread cases exist
  from commit 8 on, so a bisect over the first seven does not run them.
- The stage runner's last cases, `test_fuzz_num_ssm_stage_wide`, are the ones
  at 12 and 16 threads. If they are what fails here, that is *A wrong product*
  below, on a product small enough to repeat in a minute.

## Step 2: what the scratch volume does

No araucaria needed. Use a file of 1.5x the RAM `free` shows, in MB (36864 for
24 GB), and as much free space in `$D`.

```bash
cc -O2 -o /tmp/diskbw stall-kit/diskbw.c -pthread
{
# dir, file MB, element KB, threads, pattern, flags, ops
/tmp/diskbw $D 36864 480 1  seq  -  ww     # one writer: new blocks, then a rewrite
/tmp/diskbw $D 36864 480 16 seq  -  ww     # 16 writers, buffered
/tmp/diskbw $D 36864 480 16 seq  F  ww     # 16 writers, a file each
/tmp/diskbw $D 36864 480 16 wide -  wxx    # strided; x = half read while half write
/tmp/diskbw $D 36864 480 16 wide p  wxx    # the same, file preallocated
/tmp/diskbw $D 36864 480 16 wide s  wxx    # the same, writeback started per element
/tmp/diskbw $D 36864 480 16 wide n  wxx    # the same, O_DIRECT
/tmp/diskbw $D 36864 480 16 wide np wxx    # O_DIRECT, preallocated
/tmp/diskbw $D 36864 16384 16 seq - wxx    # 16 MB transfers, buffered
} 2>&1 | tee $L/diskbw.txt
```

480 KB is one transform element at 4G-limb operands, `wide` is the access
pattern of a strided pass, and in an `x` pass half the threads read while the
other half write, as in a transform pass.

What each comparison says:

- `1 seq`, first `w` against the second: the cost of blocks the file never had.
  A large gap points at allocation, in ext4 or in the VHDX.
- `16 seq` against `1 seq`: whether more writers on one file help at all.
- `16 seq F` against `16 seq -`: whether it is the one file that holds them
  back. A transform array is a single file.
- `16 wide -`: what a strided pass gets today. `in-call` near 16 with a rate far
  under the best row is time lost inside the kernel, not at the device.
- `s` and `n` against `-`: whether starting writeback early, or going past the
  page cache, gets closer to the device.

Mac reference, 8 threads, 416 KB elements, 12 GB file: one sequential writer
2.1 GB/s; buffered strided writes 0.9 GB/s; buffered mixed 0.76 GB/s each way;
uncached mixed 1.1 GB/s each way.

## Step 3: instrumented multiplies

### Two probes

One from the code as it was at `ae5e242`, one from this branch. The harness
goes in once and stays until the last probe is built:

```bash
git apply stall-kit/probe-harness.patch

git apply stall-kit/probe-hooks.patch
make clean && make build FLAGS_EXTRA="-DARAUCARIA_PROF -DASSERT_VERBOSE"
cp src/main.out ~/probe_fixed
git apply -R stall-kit/probe-hooks.patch

git checkout ae5e242 -- lib/num/code.c
git apply stall-kit/probe-hooks-baseline.patch
make clean && make build FLAGS_EXTRA="-DARAUCARIA_PROF -DASSERT_VERBOSE"
cp src/main.out ~/probe_base
git checkout HEAD -- lib/num/code.c
```

`src/probe.c` has only been through clang, and its Linux half (the sampler that
reads `/proc`) has never run. If GCC rejects something there, add `-Wno-error`
to `FLAGS_EXTRA` for these builds only. If the sampler misbehaves, fix it in
`src/probe.c`: it is test tooling, not library code, and the fix then holds for
every probe built after it. If step 1 made you change `lib/num/code.c`, apply
the hook patches with `git apply --3way`.

When the last probe is built, take the harness out again:

```bash
git apply -R stall-kit/probe-harness.patch && make clean
git status --short          # must print nothing
```

If you edited `src/probe.c`, that reverse will not apply: keep your copy with
the logs and remove it by hand, `git checkout -- src/main.c` and
`rm src/probe.c lib/num/prof.h`.

### One run

```bash
# run.sh <name> <probe binary> <limbs per operand> <T> <B> [VAR=value ...]
stall-kit/run.sh s_fixed ~/probe_fixed 30000000 1 256
```

`run.sh` runs the probe with every thread the machine has (`THREADS` overrides
it), saves its output as `$L/NAME.stdout.txt`, the analysis as
`$L/NAME.report.txt` and what `dmesg` had to say about I/O errors as
`$L/NAME.dmesg.txt`, and prints the result lines.

The probe multiplies two random operands made from a fixed seed and checks the
product's residue mod 2^64 - 59 against the product of the operands' residues,
so every run ends in `VERIFY ok` or `VERIFY FAIL`, and two runs of one size must
print the same residue. `tail -f` on the stdout file shows a line as each phase
ends.

### Small products first

Before anything large, the same check over a matrix of small operands, 300K to
30M limbs at 12, 16, 24 and 32 threads with six budgets, staged and not, each
compared with the all-RAM product. About 3 minutes a line on the Mac, and each
must end in `ALL OK`:

```bash
for bin in ~/probe_fixed ~/probe_base; do for op in mul sqr; do
    PROBE_DISK_PATH=$D python3 stall-kit/check.py $bin wide $op | tee -a $L/check.txt
done; done
```

This is the first time the staged path runs at 16 threads on x86-64 with a
check on the product. A failure here is small and quick to repeat: note the
line it printed, run that one configuration again, and treat it as *A wrong
product* below.

### The runs, in this order

| Name | Binary | Limbs | T | B | Free in `$D` | Why |
| --- | --- | --- | --- | --- | --- | --- |
| `s_base`, `s_fixed` | each | 30000000 | 1 | 256 | 3G | the tooling works here (under a minute) |
| `a_base` | base | 1073741824 | 4096 | `$B` | 75G | 8 GB operands, old code |
| `a_fixed` | fixed | 1073741824 | 4096 | `$B` | 75G | the same on the branch |
| `a_fixed_b` | fixed | 1073741824 | 4096 | 2400 | 75G | two sweeps per transform, not three |
| `b_base` | base | 4000000000 | `$T` | `$B` | 275G | the size the stalls were seen at |
| `b_fixed` | fixed | 4000000000 | `$T` | `$B` | 275G | the same on the branch |
| `b_fixed_v` | fixed | 4000000000 | `$T` | `$B` | 275G | again, with `PROBE_IOVERIFY=1` as the last argument of `run.sh` |
| `b_fixed_b` | fixed | 4000000000 | `$T` | 4200 | 275G | two sweeps |

Free space is as `df -h` counts it: two transform arrays and one operand, the
most a multiply holds at once. The `a_` rows use a 4 GB threshold so the
operands go to disk too: at pinhao's threshold two 8 GB operands would sit in
RAM and crowd out the page cache. The Mac took 8 to 13 minutes for an `a_` run.
In September one `b_` run took 5780 to 8123 s on this box.

Budgets: a staged pass fuses `floor(log2(B / threads / element size))` stages.
At 16 threads 1600 MB is 100 MB per worker, 7 stages at either size, so the 16
or 18 stages of a transform take three passes. 2400 MB at the `a_` size and
4200 MB at the `b_` size make it two. pinhao hands every join `mem_max /
n_process` whatever thread count it runs with, so its top joins run 16 threads
on a sixteenth of the band. If the `_b` runs show that costs much, that is a
change for pinhao, not for this repo: report it, do not make it.

### Reading a report

- The `thread time:` line is the headline: the share of the 16 threads' time on
  a CPU, inside `pread`/`pwrite`, and in neither.
- The phase table says where. `idle` threads are blocked on a page fault, a
  lock or a join. `in-io` threads are inside a read or write call, which on
  Linux includes a `pwrite` held back by dirty-page throttling.
- The stall table counts the seconds with both the CPU and the device under a
  threshold, and what the threads were doing meanwhile. The device threshold is
  600 MB/s, sized for the Mac: pass `--stall-disk` with about a third of the
  best rate step 2 showed.
- The `linux:` line and the last columns of `NAME.samples.tsv` say what the
  kernel was doing: `iowait`, `psi_io_*`, `psi_mem_*`, `dirty_mb`, `wback_mb`,
  `dev_util`. Threads in `pwrite` with `dev_util` well under 1 are held up
  before the device. `psi_mem_full` above zero is reclaim stalling threads.
- With `--rounds` the last table has one row per pass: the slowest, the average
  and the fastest worker, and how each worker's time splits into reading,
  writing, the pad and the pointwise step.
- The Linux columns come from code that has never run. If they are all zero or
  make no sense, say so, and log `iostat -x 1` and `vmstat 1` next to a run.

On the Mac the fixed code looks like this at 8 GB: every pass has all threads
inside reads and writes at the device's rate, the first inverse pass is all CPU,
and nothing is idle. The old code shows the depad with 7 of 8 threads idle and
20,000 page-ins a second, and a third sweep of the array in every transform.

### A wrong product

It shows as an assertion from `ssm_product_count_check`, or as `VERIFY FAIL`.
Either way:

1. Keep the stdout file, the report and `dmesg`, and note the free space in `$D`
   and on the Windows drive that holds the VHDX at that moment.
2. Run the same command again. The same wrong residue twice means the
   arithmetic is wrong for that input. A different outcome means a race or the
   storage.
3. Run it once more with `PROBE_IOVERIFY=1`. That keeps a hash of every element
   the staged transform writes and aborts with an `IOVERIFY` line on the first
   one that reads back different, saying how many of its limbs are zero. A hit
   means the file did not give back what was written to it, which the
   arithmetic cannot do to itself. A clean run prints `IOVERIFY ok` with the
   number of elements checked. It covers the transform arrays, not the depad's
   reads, the operands or the product.
4. Try the same size with 8 threads (`THREADS=8` in front of `run.sh`), and
   with `~/probe_base`, to see whether the thread count or the old code changes
   it.

## Step 4: only if steps 2 and 3 point there

`direct-io.patch` moves staged transform arrays past the page cache. On the Mac
it gained nothing (460 s against 457 s at 8 GB, 117 s against 100 s at 2 GB).
Try it here only if the `n` rows of step 2 clearly beat the `-` rows and step 3
shows passes with threads inside `pwrite` while the device has room.

```bash
git apply stall-kit/direct-io.patch stall-kit/probe-hooks-direct-io.patch
make clean && make build \
    FLAGS_EXTRA="-DARAUCARIA_PROF -DASSERT_VERBOSE -DPROBE_HAS_DIRECT_IO"
cp src/main.out ~/probe_direct
git apply -R stall-kit/probe-hooks-direct-io.patch stall-kit/direct-io.patch
stall-kit/run.sh a_direct ~/probe_direct 1073741824 4096 $B PROBE_DIRECT=1
```

This needs the harness of step 3 applied. Its hooks leave out the depad rounds,
and `PROBE_IOVERIFY` checks nothing in this build.

Without `PROBE_DIRECT` the same binary runs buffered; the variable counts as set
even when empty.

`split-all-workers.patch` does nothing for pinhao as it is: its scheduler only
grants powers of two.

## Leads, none of them verified

What the Mac could not test, each with what would show it:

- **Writers to one file take turns.** On ext4 a buffered write holds the file's
  inode lock, and a writer throttled for dirty pages sleeps holding it, so 16
  threads writing one array can move at the pace of one throttled writer. It
  would show as threads inside `pwrite`, little CPU and a device with room, and
  in step 2 as `F`, `s` or `n` rows well above the plain ones.
- **The budget.** Three sweeps of the array per transform where two fit, as the
  `_b` runs measure.
- **WSL taking memory back.** `autoMemoryReclaim` in `.wslconfig` drops page
  cache from a guest it takes for idle, and an I/O-bound pass uses little CPU.
  It would show as `file_mb` falling in the middle of a pass.
- **The VHDX.** Blocks the file never had may cost more than rewrites: step 2's
  first `w` against its second. And a host drive that fills up loses writes,
  which would also explain the wrong products: `dmesg` and `PROBE_IOVERIFY`.
- **16 workers.** Until commit 8 nothing in `make test` staged a transform with
  more than 6 threads. On the Mac 12 to 32 threads give right products, and CI
  passes the new 12- and 16-thread cases on x86-64; *Small products first*
  takes it further on this box.

## What to report

- Step 0 output.
- Step 1: pass or fail, and any diff it took to build.
- Step 2: `diskbw.txt`.
- Step 3: `check.txt`, every `NAME.report.txt`, and whether each product was
  right.
- For the stall itself: in which phase the CPU and the device were both idle,
  what the threads were blocked on, and whether the branch changes it.
- Whether the branch is faster here, run by run.
