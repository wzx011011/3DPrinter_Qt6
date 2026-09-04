#!/usr/bin/env python3
"""Split a git unified diff into a filtered patch keeping hunks whose content
matches a regex (excluding the diff header lines of the hunk). Usage:
  python split_hunks.py <file> <regex> <out.patch>
"""
import re
import subprocess
import sys

path, pattern, out = sys.argv[1], sys.argv[2], sys.argv[3]
diff = subprocess.run(
    ["git", "diff", "--", path], capture_output=True, text=True, encoding="utf-8"
).stdout
rx = re.compile(pattern)
lines = diff.splitlines(keepends=True)
header, hunks, current = [], [], None
for line in lines:
    if line.startswith("@@"):
        if current:
            hunks.append(current)
        current = [line]
    elif current is not None:
        current.append(line)
    else:
        header.append(line)
if current:
    hunks.append(current)

kept = []
for h in hunks:
    body = "".join(l for l in h if l[:1] in "+- \t")
    if rx.search(body):
        kept.append(h)

with open(out, "w", encoding="utf-8", newline="") as f:
    f.write("".join(header))
    for h in kept:
        f.write("".join(h))
print(f"{path}: kept {len(kept)}/{len(hunks)} hunks -> {out}")
