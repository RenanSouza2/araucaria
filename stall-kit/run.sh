#!/bin/bash
# One probed multiply, with its report and the kernel's complaints next to it.
#
# run.sh <name> <probe binary> <limbs per operand> <threshold MB> <budget MB> [VAR=value ...]
#
# D is the scratch directory and L the log directory; both must be set. THREADS
# defaults to the machine's count and OP to mul. Extra VAR=value arguments go into
# the probe's environment. Writes L/<name>.stdout.txt, .report.txt and .dmesg.txt
# beside the probe's own .samples.tsv, .marks.tsv and .rounds.tsv.
set -u
here="$(cd "$(dirname "$0")" && pwd)"

if [ $# -lt 5 ]; then
    sed -n '2,9p' "$0"
    exit 2
fi
name=$1; bin=$2; limbs=$3; threshold=$4; budget=$5; shift 5
threads=${THREADS:-$(getconf _NPROCESSORS_ONLN)}
op=${OP:-mul}

mkdir -p "$L"
env PROBE_DISK_PATH="$D" "$@" "$bin" probe "$L/$name" "$limbs" "$threads" "$threshold" "$budget" 1 "$op" \
    > "$L/$name.stdout.txt" 2>&1
status=$?
echo "exit $status" >> "$L/$name.stdout.txt"

python3 "$here/analyze.py" "$L/$name" --rounds > "$L/$name.report.txt" 2>&1
dmesg 2>/dev/null | tail -300 \
    | grep -i -E "I/O error|lost async|EXT4-fs|blk_update|hung task|out of memory" \
    > "$L/$name.dmesg.txt"

tail -4 "$L/$name.stdout.txt"
exit $status
