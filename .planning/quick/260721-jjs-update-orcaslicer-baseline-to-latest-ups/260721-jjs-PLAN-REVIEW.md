# PLAN REVIEW: BLOCK
- PASS: the locked SHA, two-patch inventory, focused null-map test, remote-SHA check, and gate split are now explicit.
- BLOCKER: the specified submodule encoding command is infeasible now: `encoding_guard.py --all` exits 1 with `WinError 123` on the tracked quoted/non-ASCII 3MF path. Check the two changed source files explicitly instead.
- BLOCKER: a fresh root clone made before the parent gitlink commit tests the old gitlink. Required order is child push/`ls-remote`, local parent commit, clone and checkout that exact parent commit, successful submodule init, then parent publication.
- BLOCKER: Task 3 leaves the active `AGENTS.md` lock at `v7.0.1` and has no negative stale-lock command. Include active baseline consumers (at minimum `AGENTS.md` plus baseline/tracker/current inventory docs) and verify they contain no active old SHA/tag claim.
- BLOCKER: Wave 0 still displays a stat plus two selected diffs but never asserts the changed-file allowlist; compare `git diff --name-only 8b93cc5d...HEAD` exactly to `src/libslic3r/Config.hpp` and `src/libslic3r/MeshBoolean.cpp`.
