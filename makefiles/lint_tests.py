#!/usr/bin/env python3
"""Static checks over the test sources. Run from the repo root: make lint

1. Duplicate case tags. TEST_CASE_OPEN / TEST_FUZZ_CASE_OPEN tags are the only
   identifier test_log_error prints on failure, so a collision inside one test
   function makes the failing row ambiguous.

2. Limb-fixture count mismatches. The *_create_immed / *_immed helpers take a
   leading count and read exactly that many varargs. Declaring more than are
   supplied makes num_create_variadic read uninitialised stack values, which
   surfaces later as a bogus assertion failure deep inside the library.
"""

import glob
import re
import sys
from collections import Counter

FN = re.compile(r"^static void (\w+)\(")
ROW = re.compile(r"^TEST_[A-Z0-9_]+\(\s*(\d+)\s*[,)]")
DIRECT = re.compile(r"^TEST_(?:FUZZ_)?CASE_OPEN(?:_TIMEOUT)?\(\s*(\d+)\s*[,)]")

TUPLE = re.compile(r"\((\s*\d+\s*,[^()]*?)\)", re.S)
TOKEN = re.compile(r"^[0-9A-Za-z_x]+$")

# Only hand-written limb blocks are checked. Short tuples are indistinguishable
# from ordinary macro argument lists and are easy to eyeball anyway.
MIN_FIXTURE_VALUES = 8


def duplicate_tags(path):
    found, current = {}, None
    for line in open(path):
        match = FN.match(line)
        if match:
            current = match.group(1)
            found.setdefault(current, [])
        stripped = line.strip()
        if (
            not current
            or stripped.startswith("#define")
            or stripped.startswith("#undef")
            or stripped.endswith("\\")
        ):
            continue
        match = ROW.match(stripped) or DIRECT.match(stripped)
        if match:
            found[current].append(int(match.group(1)))

    for fn, tags in found.items():
        repeated = {t: n for t, n in Counter(tags).items() if n > 1}
        if repeated:
            yield f"{path}: {fn}: duplicate case tags {repeated}"


def fixture_counts(path):
    src = open(path).read()
    for match in TUPLE.finditer(src):
        parts = [p.strip() for p in match.group(1).split(",")]
        parts = [p for p in parts if p]
        if len(parts) < 4 or not all(TOKEN.match(p) for p in parts):
            continue
        declared, actual = int(parts[0]), len(parts) - 1
        if declared != actual and actual >= MIN_FIXTURE_VALUES:
            line = src[: match.start()].count("\n") + 1
            yield f"{path}:{line}: fixture declares {declared} limbs, supplies {actual}"


def main():
    failures = []
    for path in sorted(glob.glob("lib/*/test/*.c")):
        failures.extend(duplicate_tags(path))
        failures.extend(fixture_counts(path))

    for failure in failures:
        print(failure, file=sys.stderr)

    if failures:
        print(f"\n{len(failures)} problem(s) found", file=sys.stderr)
        return 1

    print("test lint OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
