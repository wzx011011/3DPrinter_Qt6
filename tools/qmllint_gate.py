#!/usr/bin/env python3
"""qmllint ratchet gate (Qt 6.12 modernization plan, phase 1).

The auto-generated ``owzx_qml_qmllint_json`` CMake target lints the whole OWzx
QML module and writes one JSON report. This gate re-checks that report against
the ratchet allowlist in ``docs/qmllint-baseline/allowed.json``:

  * key   = ``<repo-relative file>|<warning id>`` (robust to line shifts)
  * value = allowed warning count for that pair

It fails when any (file, category) count EXCEEDS its allowance, when a file
not in the allowlist shows warnings, or when the report itself is missing.
Counts may only go DOWN; shrinking counts are reported and ``--update``
rewrites the allowlist to the new (lower) state so the ratchet never loosens.

Usage:
    python3 tools/qmllint_gate.py --json build/owzx_qml_qmllint.json \
        --allowlist docs/qmllint-baseline/allowed.json [--update]
"""

import argparse
import collections
import json
import os
import sys

# Categories that indicate structural damage rather than ordinary lint debt:
# any occurrence fails the gate outright, allowlist or not.
FATAL_CATEGORIES = {"syntax"}


def collect(json_path, repo_root):
    with open(json_path, encoding="utf-8") as f:
        data = json.load(f)
    counts = collections.Counter()
    fatal = []
    total = 0
    for file_entry in data.get("files", []):
        fname = os.path.relpath(file_entry.get("filename", ""), repo_root)
        for warning in file_entry.get("warnings", []):
            total += 1
            wid = warning.get("id", "?")
            counts[f"{fname}|{wid}"] += 1
            if wid in FATAL_CATEGORIES:
                fatal.append(f"{fname}:{warning.get('line')} [{wid}]")
    return counts, fatal, total


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--json", required=True, help="qmllint JSON report")
    ap.add_argument("--allowlist", required=True, help="ratchet allowlist JSON")
    ap.add_argument("--update", action="store_true",
                    help="rewrite the allowlist to the current counts "
                         "(only allowed when nothing grew)")
    args = ap.parse_args()

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if not os.path.isfile(args.json):
        print(f"GATE FAIL: qmllint report not found: {args.json}\n"
              f"(build target owzx_qml_qmllint_json first)")
        return 2

    with open(args.allowlist, encoding="utf-8") as f:
        allowed = json.load(f)

    counts, fatal, total = collect(args.json, repo_root)

    if fatal:
        print("GATE FAIL: syntax-level QML errors refuse to parse:")
        for line in fatal:
            print(f"  {line}")
        return 2

    grown, shrunk = [], []
    for key, n in sorted(counts.items()):
        a = allowed.get(key, 0)
        if n > a:
            grown.append((key, a, n))
        elif n < a:
            shrunk.append((key, a, n))

    if grown:
        print(f"GATE FAIL: {total} warnings total; regressions detected:")
        for key, a, n in grown:
            print(f"  +{n - a}  {key}  ({a} allowed -> {n} found)")
        print("Fix the new warnings, or justify + regenerate with "
              "--update (never to loosen existing entries).")
        return 1

    for key, a, n in shrunk:
        print(f"  improved: {key}  ({a} -> {n})")
    msg = (f"GATE PASS: {total} warnings, all within ratchet "
           f"({sum(a - n for _, a, n in shrunk)} removed this cycle)")
    if shrunk:
        msg += "; run --update to tighten the allowlist"
    print(msg)
    if args.update:
        new_allow = {k: v for k, v in sorted(counts.items()) if v > 0}
        with open(args.allowlist, "w", encoding="utf-8") as f:
            json.dump(new_allow, f, indent=1, ensure_ascii=False)
            f.write("\n")
        print(f"allowlist updated: {len(new_allow)} entries "
              f"(was {len(allowed)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
