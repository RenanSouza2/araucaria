#!/usr/bin/env python3
"""Summarise one probe run: where the wall time went, phase by phase.

usage: analyze.py <run dir or log prefix> [--threads N] [--rounds]
                  [--stall-cpu FRAC] [--stall-disk MBS] [--stall-queue N]

With a log prefix it reads <prefix>.samples.tsv, .marks.tsv and .rounds.tsv, and
<prefix>.stdout.txt when the probe's output was saved there.
"""

import argparse
import bisect
import collections
import os
import re


def read_tsv(path):
    if not os.path.exists(path):
        return []
    rows = []
    with open(path) as fp:
        header = fp.readline().rstrip("\n").split("\t")
        for line in fp:
            cols = line.rstrip("\n").split("\t")
            if len(cols) != len(header):
                continue
            row = {}
            for k, v in zip(header, cols):
                try:
                    row[k] = float(v)
                except ValueError:
                    row[k] = v
            rows.append(row)
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir")
    ap.add_argument("--stall-cpu", type=float, default=0.4)
    ap.add_argument("--stall-disk", type=float, default=600.0)
    ap.add_argument("--stall-queue", type=float, default=4.0)
    ap.add_argument("--rounds", action="store_true")
    ap.add_argument("--threads", type=int, help="thread count of the run, when there is no stdout.txt")
    a = ap.parse_args()

    d = a.run_dir
    samples = read_tsv(os.path.join(d, "probe.samples.tsv"))
    disk = read_tsv(os.path.join(d, "disk.tsv"))
    marks = read_tsv(os.path.join(d, "probe.marks.tsv"))
    rounds = read_tsv(os.path.join(d, "probe.rounds.tsv"))
    stdout_path = os.path.join(d, "stdout.txt")
    meta = open(os.path.join(d, "meta.txt")).read() if os.path.exists(os.path.join(d, "meta.txt")) else ""

    if not samples and os.path.isfile(d + ".samples.tsv"):
        # a bare log prefix, as the probe writes when run by hand
        samples = read_tsv(d + ".samples.tsv")
        marks = read_tsv(d + ".marks.tsv")
        rounds = read_tsv(d + ".rounds.tsv")
        stdout_path = d + ".stdout.txt"

    stdout = open(stdout_path).read() if os.path.exists(stdout_path) else ""
    m = re.search(r"threads (\d+)", stdout)
    threads = a.threads or (int(m.group(1)) if m else 8)

    print(f"== {os.path.basename(d.rstrip('/'))}")
    for line in meta.splitlines():
        if line.startswith(("args", "ssm", "power", "date")):
            print("   " + line)
    for line in stdout.splitlines():
        if line.startswith(("probe:", "RESULT", "VERIFY", "generate")) or "IOVERIFY" in line or "Assertion" in line:
            print("   " + line.strip())

    # device rows keyed by epoch; each sample row takes the nearest one
    disk_epochs = [r["epoch"] for r in disk]

    def dev_at(epoch):
        if not disk:
            return None
        i = bisect.bisect_left(disk_epochs, epoch)
        cands = [j for j in (i - 1, i) if 0 <= j < len(disk)]
        j = min(cands, key=lambda x: abs(disk_epochs[x] - epoch))
        return disk[j] if abs(disk_epochs[j] - epoch) < 2.5 else None

    # on Linux the probe samples the device itself; on macOS run_probe.py logs it
    for s in samples:
        dv = dev_at(s["epoch"])
        s["dev_rd"] = dv["dev_rd_mb"] if dv else s.get("dev_rd_mb", 0.0)
        s["dev_wr"] = dv["dev_wr_mb"] if dv else s.get("dev_wr_mb", 0.0)
        s["inflight"] = dv["inflight"] if dv else s.get("dev_queue", 0.0)

    mul = [s for s in samples if s["label"] == "multiply"]
    total = len(mul)
    if not mul and not marks:
        print("   no multiply samples and no phase marks")
        return

    # ---- phase table from the marks
    if marks:
        print()
        print(
            f"{'stage':6}{'phase':14}{'wall s':>9}{'%':>6}{'cpu':>6}{'in-io':>6}{'idle':>6}"
            f"{'rd GB':>8}{'wr GB':>8}{'dev rd':>8}{'dev wr':>8}{'infl':>9}{'pgin/s':>9}"
        )
        sum_wall = sum(mk["wall"] for mk in marks)
        agg = collections.OrderedDict()
        for mk in marks:
            t1 = mk["t"]
            t0 = t1 - mk["wall"]
            win = [s for s in mul if t0 < s["t"] <= t1 + 0.5]
            key = (mk["stage"], mk["label"])
            e = agg.setdefault(key, collections.defaultdict(float))
            e["wall"] += mk["wall"]
            e["cpu_s"] += mk["user"] + mk["sys"]
            e["io_s"] += mk["rd_s"] + mk["wr_s"]
            e["rd_gb"] += mk["rd_gb"]
            e["wr_gb"] += mk["wr_gb"]
            e["pageins"] += mk["pageins"]
            e["n"] += len(win)
            e["dev_rd"] += sum(s["dev_rd"] for s in win)
            e["dev_wr"] += sum(s["dev_wr"] for s in win)
            e["inflight"] += sum(s["inflight"] for s in win)
        tot_cpu = 0.0
        tot_io = 0.0
        tot_idle = 0.0
        for (stage, label), e in agg.items():
            wall = e["wall"]
            tot_cpu += e["cpu_s"]
            tot_io += e["io_s"]
            tot_idle += max(0.0, threads * wall - e["cpu_s"] - e["io_s"])
            if wall < 0.005 * sum_wall and wall < 1.0:
                continue
            cpu = e["cpu_s"] / wall if wall else 0
            io = e["io_s"] / wall if wall else 0
            n = e["n"] or 1
            print(
                f"{stage:6}{label:14}{wall:9.1f}{100 * wall / sum_wall:6.1f}{cpu:6.2f}{io:6.2f}"
                f"{max(0.0, threads - cpu - io):6.2f}{e['rd_gb']:8.1f}{e['wr_gb']:8.1f}"
                f"{e['dev_rd'] / n:8.0f}{e['dev_wr'] / n:8.0f}{e['inflight'] / n:9.1f}"
                f"{e['pageins'] / wall if wall else 0:9.0f}"
            )
        print(f"{'':6}{'total':14}{sum_wall:9.1f}")
        budget = threads * sum_wall
        print(
            f"thread time: {100 * tot_cpu / budget:.0f}% on a CPU, {100 * tot_io / budget:.0f}% inside pread/pwrite,"
            f" {100 * tot_idle / budget:.0f}% in neither ({tot_idle:.0f} of {budget:.0f} thread-seconds)"
        )

    # ---- stall table from the samples
    print()
    if not mul:
        print("the multiply ended before the first sample: no stall table")
        return
    if not any(s["dev_rd"] or s["dev_wr"] for s in mul):
        print("no device samples in this run: stalled and idle below only reflect the CPU")
    print(
        f"stalled = process cpu < {a.stall_cpu:.2f} x {threads} threads AND device < {a.stall_disk:.0f} MB/s;"
        f"  idle = stalled AND fewer than {a.stall_queue:.0f} requests in flight at the device"
    )
    by_phase = collections.OrderedDict()
    for s in mul:
        key = (s["stage"], s["phase"])
        e = by_phase.setdefault(key, collections.defaultdict(float))
        dev = s["dev_rd"] + s["dev_wr"]
        e["n"] += 1
        e["cpu"] += s["cpu"]
        e["dev"] += dev
        stalled = s["cpu"] < a.stall_cpu * threads and dev < a.stall_disk
        if stalled and s["inflight"] < a.stall_queue:
            e["idle"] += 1
        if stalled:
            e["stall"] += 1
            e["stall_cpu"] += s["cpu"]
            e["stall_dev"] += dev
            e["stall_in_rd"] += s["in_rd"]
            e["stall_in_wr"] += s["in_wr"]
            e["stall_act"] += s["act"]
            e["stall_pgin"] += s["pgin_s"]
            e["stall_kern"] += s["sys_kern"]
            e["stall_infl"] += s["inflight"]
    print(
        f"{'stage':6}{'phase':14}{'samples':>8}{'idle':>6}{'%':>6}{'stalled':>8}{'%':>6}  while stalled:"
        f"{'cpu':>6}{'dev':>7}{'infl':>6}{'act':>6}{'in_rd':>6}{'in_wr':>6}{'pgin/s':>8}{'kern':>6}"
    )
    tot_stall = 0
    tot_idle = 0
    for (stage, phase), e in by_phase.items():
        st = e["stall"]
        tot_stall += st
        tot_idle += e["idle"]
        if e["n"] < 2 and not st:
            continue
        k = st or 1
        print(
            f"{stage:6}{phase:14}{e['n']:8.0f}{e['idle']:6.0f}{100 * e['idle'] / e['n']:6.1f}"
            f"{st:8.0f}{100 * st / e['n']:6.1f}  {'':14}"
            f"{e['stall_cpu'] / k:6.2f}{e['stall_dev'] / k:7.0f}{e['stall_infl'] / k:6.1f}"
            f"{e['stall_act'] / k:6.2f}{e['stall_in_rd'] / k:6.2f}{e['stall_in_wr'] / k:6.2f}"
            f"{e['stall_pgin'] / k:8.0f}{e['stall_kern'] / k:6.2f}"
        )
    print(
        f"{'':6}{'total':14}{total:8.0f}{tot_idle:6.0f}{100 * tot_idle / total:6.1f}"
        f"{tot_stall:8.0f}{100 * tot_stall / total:6.1f}"
    )

    if any(s.get("psi_io_full", 0.0) or s.get("iowait", 0.0) for s in mul):
        n = len(mul)
        print(
            f"linux: iowait {sum(s['iowait'] for s in mul) / n:.2f} cores  "
            f"psi io some/full {sum(s['psi_io_some'] for s in mul) / n:.2f}/{sum(s['psi_io_full'] for s in mul) / n:.2f}  "
            f"psi mem some/full {sum(s['psi_mem_some'] for s in mul) / n:.2f}/{sum(s['psi_mem_full'] for s in mul) / n:.2f}  "
            f"dirty peak {max(s['dirty_mb'] for s in mul):.0f} MB  writeback peak {max(s['wback_mb'] for s in mul):.0f} MB  "
            f"device util {sum(s['dev_util'] for s in mul) / n:.2f}"
        )

    therm = sorted({int(s["therm"]) for s in mul})
    memlvl = sorted({int(s["memlvl"]) for s in mul})
    lpm = sorted({int(s["lpm"]) for s in mul})
    swap = (min(s["swap_mb"] for s in mul), max(s["swap_mb"] for s in mul))
    comp = max(s["comp_mb"] for s in mul)
    print(
        f"thermal levels seen {therm}  memory pressure levels {memlvl}  lpm {lpm}"
        f"  swap {swap[0]:.0f}-{swap[1]:.0f} MB  compressor peak {comp:.0f} MB"
    )

    # ---- per round worker balance
    if rounds and a.rounds:
        print()
        print(
            f"{'stage':6}{'kind':10}{'round':>6}{'r':>3}{'gl':>7}{'n':>3}{'wall min':>9}{'avg':>8}{'max':>8}"
            f"{'cpu/wall':>9}{'rd s':>8}{'wr s':>8}{'pad s':>7}{'pw s':>8}{'rest s':>8}{'wait':>7}"
        )
        groups = collections.OrderedDict()
        for r in rounds:
            pass_like = r["kind"] in ("fwd-pass", "inv-pass") or r["kind"].startswith("depad")
            key = (r["stage"], r["kind"], "-" if pass_like else int(r["round"]), int(r["r"]), int(r["gl"]))
            groups.setdefault(key, []).append(r)
        for (stage, kind, rnd, rr, gl), rs in groups.items():
            walls = [r["wall"] for r in rs]
            n = len(rs)
            avg = sum(walls) / n
            mx = max(walls)
            cpu = sum(r["cpu"] for r in rs) / sum(walls) if sum(walls) else 0
            rd = sum(r["rd_s"] for r in rs) / n
            wr = sum(r["wr_s"] for r in rs) / n
            pad = sum(r["pad_s"] for r in rs) / n
            pw = sum(r["pw_s"] for r in rs) / n
            rest = avg - rd - wr - pad - pw
            wait = sum(mx - w for w in walls) / n
            print(
                f"{stage:6}{kind:10}{str(rnd):>6}{rr:3}{gl:7}{n:3}{min(walls):9.1f}{avg:8.1f}{mx:8.1f}"
                f"{cpu:9.2f}{rd:8.1f}{wr:8.1f}{pad:7.1f}{pw:8.1f}{rest:8.1f}{wait:7.1f}"
            )


if __name__ == "__main__":
    main()
