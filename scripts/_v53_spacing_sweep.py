#!/usr/bin/env python
# Phase 172 (CL-02): dialog spacing sweep.
# Maps hand-rolled spacing/margin values in dialogs/*.qml to the nearest
# Theme.spacing* token. Mirror of Phase 162/163 mechanical color/typography
# sweeps. Skip Theme.spacing*/panelPadding already-used values.
#
# Spacing scale: XS=4 SM=6 MD=8 LG=12 XL=16 XXL=24
# Targets: `spacing: N`, `leftMargin/rightMargin/topMargin/bottomMargin: N`,
#          `margins: N`, `Layout.margins: N`.
# Map (within ±1 tolerance, exact first):
#   0/1/2/3/4 → spacingXS (4)
#   5/6/7 → spacingSM (6)
#   8/9/10 → spacingMD (8)
#   11/12/13/14 → spacingLG (12)
#   15/16/17/18 → spacingXL (16)
#   20/24 → spacingXXL (24)
# Skip values >24 (probably intentional non-spacing layout values) and the
# value 1 (often a hairline border, not spacing).

import os, re, glob

SPACING_MAP = [
    # (set_of_values, token_name)
    ({0, 2, 3, 4}, "spacingXS"),     # 4
    ({5, 6, 7}, "spacingSM"),       # 6
    ({8, 9, 10}, "spacingMD"),      # 8
    ({11, 12, 13, 14}, "spacingLG"),# 12
    ({15, 16, 17, 18}, "spacingXL"),# 16
    ({20, 24}, "spacingXXL"),       # 24
]

def nearest_token(n):
    for values, tok in SPACING_MAP:
        if n in values:
            return tok
    return None

# Properties to scan (only those that take pixel values for spacing/margin).
# Match patterns like `spacing: 8` (with optional surrounding whitespace).
PROPERTIES = [
    "spacing", "leftMargin", "rightMargin", "topMargin", "bottomMargin",
    "margins", "Layout.leftMargin", "Layout.rightMargin",
    "Layout.topMargin", "Layout.bottomMargin", "Layout.margins",
]

total_replaced = 0
files_changed = 0
for path in glob.glob("src/qml_gui/dialogs/*.qml"):
    with open(path, encoding="utf-8") as f:
        src = f.read()
    original = src
    for prop in PROPERTIES:
        # Match `<prop>: <int>` — only literal integers, not Theme.* or expressions.
        pattern = re.compile(r"\b" + re.escape(prop) + r":\s*(\d+)\s*$", re.MULTILINE)
        def repl(m):
            nonlocal_count[0] += 1
            n = int(m.group(1))
            tok = nearest_token(n)
            if tok is None:
                return m.group(0)
            return prop + ": Theme." + tok
        nonlocal_count = [0]
        src = pattern.sub(repl, src)
        total_replaced += nonlocal_count[0]
    if src != original:
        with open(path, "w", encoding="utf-8") as f:
            f.write(src)
        files_changed += 1

print(f"Total spacing replacements: {total_replaced}")
print(f"Dialog files modified: {files_changed}")
