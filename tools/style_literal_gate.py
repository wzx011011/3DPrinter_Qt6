#!/usr/bin/env python3
"""Style-literal ratchet gate (UI consistency batch, 2026-10-25).

Theme.qml (src/qml_gui/Theme.qml) is the single source of truth for design
tokens. This gate scans the QML sources for style values that bypass the
token scales and ratchets them against docs/style-baseline/allowed.json:

  * spacing-literal  — spacing/margins/*Margin/*Padding/*Spacing assigned an
                       integer that is neither 0/1 (hairline/no-gap) nor on
                       the 7-step scale 2/4/6/8/12/16/24
  * radius-literal   — radius assigned an integer that is neither 0 nor on
                       the radius scale 2/4/6/8/12/16/24
  * hex-color        — #rrggbb / #aarrggbb literals (Theme.qml exempt)
  * named-color      — SVG named color literals other than "transparent"
  * font-literal     — font.pixelSize assigned a raw integer
  * qt-colorfn      — Qt.rgba / Qt.hsla / Qt.darker / Qt.lighter / Qt.tint

Key = ``<repo-relative file>|<category>``, value = allowed count. The gate
fails when any count EXCEEDS its allowance or when a file not in the
allowlist shows findings; counts may only go down. ``--update`` rewrites the
allowlist to the new (lower) state.

A finding can be exempted by ending its source line with a comment carrying
``style-exempt`` (e.g. ``spacing: 10  // style-exempt: upstream AddSpacer``)
— used for upstream-measured literals the restoration map pins verbatim.

Usage:
    python3 tools/style_literal_gate.py [--update]
"""

import argparse
import collections
import json
import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QML_ROOT = os.path.join(REPO_ROOT, "src", "qml_gui")
ALLOWLIST = os.path.join(REPO_ROOT, "docs", "style-baseline", "allowed.json")

# Theme.qml holds the literal definitions by design.
FILE_EXEMPT = {os.path.join("src", "qml_gui", "Theme.qml")}

SPACING_SCALE = {2, 4, 6, 8, 12, 16, 24}
RADIUS_SCALE = {2, 4, 6, 8, 12, 16, 24}

SPACING_PROPS = (
    "spacing|margins|leftMargin|rightMargin|topMargin|bottomMargin|"
    "leftPadding|rightPadding|topPadding|bottomPadding|"
    "columnSpacing|rowSpacing"
)
# TAIL anchors the value as the COMPLETE right-hand side (optionally followed
# by a trailing comment, ``;`` or ``}`` for single-line multi-property forms) —
# never the head of an arithmetic expression like ``18 + headerRow.height``.
RE_TAIL = r"(?=\s*(?://[^\n]*)?$|\s*[;}])"
RE_SPACING = re.compile(rf"\b(?:{SPACING_PROPS})\s*:\s*(-?\d+){RE_TAIL}")
RE_RADIUS = re.compile(rf"\bradius\s*:\s*(-?\d+){RE_TAIL}")
RE_HEX = re.compile(r"#[0-9a-fA-F]{3}\b|#[0-9a-fA-F]{6}\b|#[0-9a-fA-F]{8}\b")
RE_NAMED = re.compile(
    r'\bcolor\s*:\s*"(?!transparent\b)(white|black|red|green|blue|gray|grey|'
    r'lightgray|lightgrey|darkgray|darkgrey|silver|orange|yellow|purple|'
    r'magenta|cyan|brown|pink)"')
RE_FONT = re.compile(rf"\bfont\.pixelSize\s*:\s*\d+{RE_TAIL}")
RE_QTFN = re.compile(r"\bQt\.(?:rgba|hsla|darker|lighter|tint)\s*\(")
# style-exempt marks manual adjudications; the upstream-pin alternation
# auto-exempts lines whose comment names the upstream measurement (wx sizer
# values, styles.css pins, upstream padding constants) — that numeric IS the
# restoration truth, not drift.
RE_EXEMPT = re.compile(
    r"//.*style-exempt|//.*(?:styles\.css|wx(?:LEFT|RIGHT|TOP|BOTTOM|ALL)|"
    r"upstream\s+(?:WindowPadding|CellPadding|vertical_margin|AddSpacer))")


def scan_file(path_rel):
    """Return a Counter of category -> findings for one QML file."""
    counts = collections.Counter()
    with open(os.path.join(REPO_ROOT, path_rel), encoding="utf-8") as f:
        for line in f:
            if RE_EXEMPT.search(line):
                continue
            # Hex/named-color checks look at CODE only: trailing // comments
            # routinely cite the upstream value a token replaced
            # ("color: Theme.accent  // :379 #009688"), and those citations
            # are documentation, not hardcodes.
            code = line.split("//", 1)[0]
            for m in RE_SPACING.finditer(line):
                v = int(m.group(1))
                # 0/1 = no-gap/hairline; >24 = structural offset (a position
                # or size expressed as a margin), outside the rhythm system.
                if v in (0, 1) or v in SPACING_SCALE or v > 24 or v < 0:
                    continue
                counts["spacing-literal"] += 1
            for m in RE_RADIUS.finditer(line):
                v = int(m.group(1))
                if v == 0 or v in RADIUS_SCALE:
                    continue
                counts["radius-literal"] += 1
            if RE_HEX.search(code):
                counts["hex-color"] += 1
            if RE_NAMED.search(code):
                counts["named-color"] += 1
            if RE_FONT.search(line):
                counts["font-literal"] += 1
            if RE_QTFN.search(line):
                counts["qt-colorfn"] += 1
    return counts


def collect():
    counts = collections.Counter()
    for dirpath, _dirs, files in os.walk(QML_ROOT):
        for name in files:
            if not name.endswith(".qml"):
                continue
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, REPO_ROOT)
            if rel in FILE_EXEMPT:
                continue
            for cat, n in scan_file(rel).items():
                counts[f"{rel}|{cat}"] += n
    return counts


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--update", action="store_true",
                    help="rewrite the allowlist to the current counts "
                         "(only allowed when nothing grew)")
    ap.add_argument("--bootstrap", action="store_true",
                    help="seed the allowlist from scratch (first use only; "
                         "prints the seeded total)")
    args = ap.parse_args()

    if args.bootstrap and not args.update:
        counts = collect()
        with open(ALLOWLIST, "w", encoding="utf-8") as f:
            json.dump(dict(sorted(counts.items())), f, indent=1,
                      ensure_ascii=False)
            f.write("\n")
        print(f"allowlist bootstrapped: {len(counts)} entries, "
              f"total {sum(counts.values())}")
        return 0

    with open(ALLOWLIST, encoding="utf-8") as f:
        allowed = json.load(f)

    counts = collect()

    grown, shrunk = [], []
    for key, n in sorted(counts.items()):
        a = allowed.get(key, 0)
        if n > a:
            grown.append((key, a, n))
        elif n < a:
            shrunk.append((key, a, n))
    stale = [k for k in allowed if k not in counts and allowed[k] > 0]

    total_now = sum(counts.values())
    total_allowed = sum(allowed.values())
    print(f"style-literal ratchet: now={total_now} allowed={total_allowed}")

    for key, a, n in shrunk:
        print(f"  shrunk: {key}  {a} -> {n}")
    for key in stale:
        print(f"  shrunk: {key}  {allowed[key]} -> 0")
    for key, a, n in grown:
        print(f"  GREW:  {key}  {a} -> {n}")

    if grown:
        print("GATE FAIL: style-literal counts may only go down. Fix the new "
              "hardcodes or run with --update only after sweeping them.")
        return 2

    if args.update:
        with open(ALLOWLIST, "w", encoding="utf-8") as f:
            json.dump(dict(sorted(counts.items())), f, indent=1, ensure_ascii=False)
            f.write("\n")
        print(f"allowlist updated: {len(counts)} entries, total {total_now}")
    elif shrunk or stale:
        print("note: counts dropped below the allowlist — run with --update "
              "to tighten the ratchet.")
    print("GATE PASS" if not grown else "")
    return 0


if __name__ == "__main__":
    sys.exit(main())
