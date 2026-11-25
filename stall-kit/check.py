#!/usr/bin/env python3
"""Product checks of one probe binary across sizes, thread counts and budgets.

Every run multiplies two random operands and checks the product's residue mod
2^64 - 59 against the product of the operands' residues. Each disk-backed run is
also compared with the all-RAM run of the same size and seed.

usage: check.py <probe binary> [quick|wide|full] [mul|sqr]

  quick  1, 3 and 8 threads, up to 3M limbs
  wide   12, 16, 24 and 32 threads, up to 30M limbs
  full   1 to 8 threads, up to 30M limbs

Scratch files go to PROBE_DISK_PATH, or to a temporary directory without it.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time


def lpm():
    # macOS low power mode; 0 where there is no pmset
    if not shutil.which("pmset"):
        return 0
    out = subprocess.run(["pmset", "-g"], capture_output=True, text=True).stdout
    m = re.search(r"lowpowermode\s+(\d)", out)
    return int(m.group(1)) if m else 0


def run(binary, op, prefix, env, limbs, threads, thr_mb, bud_mb, seed):
    cmd = [binary, "probe", prefix, str(limbs), str(threads), str(thr_mb), str(bud_mb), str(seed), op]
    p = subprocess.run(cmd, capture_output=True, text=True, env=env)
    m = re.search(r"VERIFY (\w+)\s+residue (\w+) expected (\w+)", p.stdout)
    if not m:
        tail = (p.stdout + p.stderr).strip().splitlines()[-3:]
        return ("CRASH", "", " | ".join(tail), p.returncode)
    return (m.group(1), m.group(2), m.group(3), p.returncode)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    binary = os.path.abspath(sys.argv[1])
    mode = sys.argv[2] if len(sys.argv) > 2 else "quick"
    op = sys.argv[3] if len(sys.argv) > 3 else "mul"

    if lpm() != 0:
        print("*** LOW POWER MODE IS ON -- not running ***")
        return 3

    if mode == "quick":
        sizes = [300_000, 1_000_000, 3_000_000]
        threads = [1, 3, 8]
        budgets = [0, 9, 16, 64]
    elif mode == "wide":
        sizes = [300_000, 1_000_000, 3_000_000, 10_000_000, 30_000_000]
        threads = [12, 16, 24, 32]
        budgets = [0, 24, 33, 64, 256, 1024]
    else:
        sizes = [100_000, 300_000, 1_000_000, 3_000_000, 10_000_000, 30_000_000]
        threads = [1, 2, 3, 5, 6, 7, 8]
        budgets = [0, 9, 12, 16, 33, 64, 256, 2048]

    tmp = tempfile.mkdtemp(prefix="stall-check-")
    prefix = os.path.join(tmp, "c")
    env = dict(os.environ, PROBE_PERIOD_MS="1000")
    if "PROBE_DISK_PATH" not in env:
        env["PROBE_DISK_PATH"] = os.path.join(tmp, "disk")
        os.makedirs(env["PROBE_DISK_PATH"])

    t0 = time.time()
    fails = 0
    runs = 0
    for limbs in sizes:
        for seed in (1, 7):
            if mode == "wide" and limbs >= 30_000_000 and seed == 7:
                continue
            # threshold 0 keeps every number in RAM
            ref = run(binary, op, prefix, env, limbs, 8, 0, 0, seed)
            runs += 1
            if ref[0] != "ok":
                print(f"FAIL ram limbs {limbs} seed {seed}: {ref}")
                fails += 1
                continue
            for th in threads:
                if limbs >= 10_000_000 and th in (1, 2, 5, 6, 12, 24):
                    continue
                for bud in budgets:
                    if seed == 7 and bud in (12, 24, 33, 1024, 2048):
                        continue
                    res = run(binary, op, prefix, env, limbs, th, 1, bud, seed)
                    runs += 1
                    if res[0] != "ok" or res[1] != ref[1]:
                        fails += 1
                        print(f"FAIL limbs {limbs} seed {seed} threads {th} budget {bud} MB: {res}  ram {ref[1]}")
            print(f"limbs {limbs:>9} seed {seed}: residue {ref[1]}  runs so far {runs}  fails {fails}  {time.time() - t0:.0f} s")
            sys.stdout.flush()

    shutil.rmtree(tmp, ignore_errors=True)
    print(f"{'ALL OK' if not fails else 'FAILURES'} ({op}, {mode}): {runs} runs, {fails} failures, {time.time() - t0:.0f} s")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
