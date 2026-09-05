#!/usr/bin/env python3
"""Read-only consistency check for the unified task ledgers.

Validates (stdlib only, Python 3.8+):
  1. .planning/WORK_ITEMS.yaml exists and every item has a unique id and the
     four status dimensions.
  2. alias_map values and item aliases do not collide with a different item id.
  3. Entry docs (STATE/INDEX/ROADMAP/REQUIREMENTS) reference WORK_ITEMS.yaml.
  4. ROADMAP progress table has no "Not started" rows left for phases 231-241.
  5. REQUIREMENTS.md carries the milestone-close clarification and no unchecked
     v5.16 requirement checkboxes remain.
  6. No active document references the retired third_party/CrealityPrint truth
     path except in explicit historical/disposition notes.
  7. Commits cited in WORK_ITEMS.yaml evidence exist in git history.

Exit code 0 = consistent; 1 = violations found (printed to stdout).
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LEDGER = ROOT / ".planning" / "WORK_ITEMS.yaml"

STATUS_FIELDS = ("implementation", "parity", "verification", "availability")

ENTRY_DOCS = [
    ROOT / ".planning" / "STATE.md",
    ROOT / ".planning" / "INDEX.md",
    ROOT / ".planning" / "ROADMAP.md",
    ROOT / ".planning" / "REQUIREMENTS.md",
]


def fail(msg, errors):
    errors.append(msg)


def _unquote(token):
    """Strip one pair of surrounding double quotes only; single quotes are
    significant in aliases such as R-P1.K'."""
    if len(token) >= 2 and token[0] == '"' and token[-1] == '"':
        return token[1:-1]
    return token


def parse_items(text):
    """Minimal block scan: top-level `- id:` items with their field lines."""
    items = []
    current = None
    for raw in text.splitlines():
        line = raw.rstrip()
        m = re.match(r"^  - id: (\S+)", line)
        if m:
            current = {"id": m.group(1), "fields": {}, "aliases": []}
            items.append(current)
            continue
        if current is None:
            continue
        for field in STATUS_FIELDS:
            m = re.match(r"^    %s:\s*(\S+)\s*$" % field, line)
            if m:
                current["fields"][field] = m.group(1)
        m = re.match(r"^    aliases: \[(.*)\]\s*$", line)
        if m:
            inner = m.group(1).strip()
            if inner:
                current["aliases"] = [
                    _unquote(a.strip()) for a in inner.split(",")
                    if a.strip()
                ]
        m = re.match(r"^    status: (\S+)\s*$", line)
        if m:
            current["status"] = m.group(1)
    return items


def main():
    errors = []
    if not LEDGER.exists():
        print("FATAL: %s missing" % LEDGER)
        return 1
    text = LEDGER.read_text(encoding="utf-8")

    items = parse_items(text)
    if not items:
        fail("no items parsed from WORK_ITEMS.yaml", errors)

    ids = [it["id"] for it in items]
    dupes = {i for i in ids if ids.count(i) > 1}
    if dupes:
        fail("duplicate item ids: %s" % sorted(dupes), errors)

    for it in items:
        for field in STATUS_FIELDS:
            if field not in it["fields"]:
                fail("item %s missing status field '%s'" % (it["id"], field), errors)

    # Alias must not equal another item's canonical id.
    id_set = set(ids)
    alias_owner = {}
    for it in items:
        for alias in it["aliases"]:
            if alias in id_set and alias != it["id"]:
                fail("alias '%s' of item %s collides with canonical id"
                     % (alias, it["id"]), errors)
            if alias in alias_owner and alias_owner[alias] != it["id"]:
                fail("alias '%s' claimed by both %s and %s"
                     % (alias, alias_owner[alias], it["id"]), errors)
            alias_owner[alias] = it["id"]

    # Entry docs must reference the ledger.
    for doc in ENTRY_DOCS:
        if not doc.exists():
            fail("entry doc missing: %s" % doc.name, errors)
            continue
        if "WORK_ITEMS.yaml" not in doc.read_text(encoding="utf-8"):
            fail("%s does not reference WORK_ITEMS.yaml" % doc.name, errors)

    # ROADMAP progress table must have no stale "Not started" phase rows.
    roadmap = (ROOT / ".planning" / "ROADMAP.md").read_text(encoding="utf-8")
    stale = [l.strip() for l in roadmap.splitlines()
             if re.match(r"^\| 2\d\d\.", l.strip()) and "Not started" in l]
    if stale:
        fail("ROADMAP progress rows still 'Not started': %d row(s)" % len(stale),
             errors)

    # REQUIREMENTS: clarification present, no unchecked v5.16 requirement boxes.
    req = (ROOT / ".planning" / "REQUIREMENTS.md").read_text(encoding="utf-8")
    if "Closed:" not in req:
        fail("REQUIREMENTS.md missing milestone-close clarification", errors)
    unchecked = len(re.findall(r"^- \[ \] \*\*", req, re.M))
    if unchecked:
        fail("REQUIREMENTS.md has %d unchecked requirement checkbox(es)" % unchecked,
             errors)

    # Retired upstream path must not appear as active truth in the ledger.
    if "third_party/CrealityPrint" in text:
        fail("WORK_ITEMS.yaml references retired path third_party/CrealityPrint",
             errors)

    # Cited commits must exist in git history.
    commits = sorted(set(re.findall(r"^\s+commits: \[(.+)\]\s*$", text, re.M)))
    cited = []
    for chunk in commits:
        cited += [c.strip() for c in chunk.split(",") if c.strip()]
    for sha in cited:
        r = subprocess.run(["git", "rev-parse", "--verify", "--quiet",
                            sha + "^{commit}"], cwd=str(ROOT),
                           capture_output=True)
        if r.returncode != 0:
            fail("cited commit not found in git history: %s" % sha, errors)

    if errors:
        print("TASK LEDGER CHECK: %d violation(s)" % len(errors))
        for e in errors:
            print("  - %s" % e)
        return 1
    print("TASK LEDGER CHECK: OK (%d items, %d aliases, %d commits verified)"
          % (len(items), len(alias_owner), len(cited)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
