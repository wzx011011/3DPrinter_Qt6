---
quick_id: 260721-jjs
slug: update-orcaslicer-baseline-to-latest-ups
status: incomplete
subsystem: upstream-baseline
tags: [orcaslicer, libslic3r, cgal, config, source-truth]
requires:
  - official OrcaSlicer upstream/main commit 8b93cc5df3347c657ce6ac9a58f6923a21c2959b
provides:
  - local child branch owzx-upstream-2026-07-21 with two reviewed compatibility deltas
  - focused null-keys_map regression coverage
  - updated active baseline documentation
affects: [prepare-context-menu, upstream-sync, reproducible-submodules]
key-files:
  created:
    - .planning/quick/260721-jjs-update-orcaslicer-baseline-to-latest-ups/260721-jjs-SUMMARY.md
  modified:
    - third_party/OrcaSlicer/src/libslic3r/Config.hpp
    - third_party/OrcaSlicer/src/libslic3r/MeshBoolean.cpp
    - tests/ViewModelSmokeTests.cpp
    - AGENTS.md
    - docs/源码真值基线.md
    - docs/源码对照迁移任务追踪.md
    - docs/v3.6-ui-inventory.md
    - docs/架构梳理_快速认知.md
    - docs/架构梳理.html
key-decisions:
  - Keep the child diff limited to Config.hpp null-map guards and MeshBoolean.cpp CGAL 5.4 compatibility.
  - Do not push the child branch or commit the parent gitlink without explicit authorization and reachability proof.
  - Do not expand the compatibility allowlist to fix the newly exposed multi-plate print-sequence regression.
metrics:
  started: 2026-07-21T07:04:46Z
  last_verified: 2026-07-22T10:09:29Z
  canonical_build_duration: 25m18s
  child_commits: 2
---

# Quick Task 260721-jjs: OrcaSlicer Baseline Summary

**Official `upstream/main` was reconstructed locally with only the reviewed Config null-map and CGAL 5.4 deltas, but the baseline gate remains incomplete because canonical verification exposes a multi-plate persistence regression and the child branch is not remotely reachable.**

## Outcome

- Created local child branch `owzx-upstream-2026-07-21` from exact official commit `8b93cc5df3347c657ce6ac9a58f6923a21c2959b`.
- Committed the Config null-`keys_map` behavior as `ad3135ce0dd08987734e629d9c1c8920f0a98e4d`.
- Committed the CGAL 5.4 hole-refinement overload compatibility as child HEAD `4cb3b9ce792f15c38fabfb4bb9700895d32b1166`.
- Added `configEnumNullKeysMapGuards()` and updated all active stale baseline declarations in the owned documentation.
- Left the parent gitlink and all root changes uncommitted, as required while the child branch is absent from the configured fork.

## Verification

| Gate | Result |
| --- | --- |
| Official base is an ancestor | PASS |
| Child diff allowlist | PASS: only `src/libslic3r/Config.hpp` and `src/libslic3r/MeshBoolean.cpp` |
| Child `diff --check` | PASS |
| Active `v7.0.1` / `0d4ac73...` claims | PASS: none in the six active baseline documents |
| Root and child encoding guard | PASS |
| `build\ViewModelSmokeTests.exe configEnumNullKeysMapGuards` | PASS, exit 0 |
| Canonical configure and build | PASS |
| PrepareSceneDataTests | PASS |
| PartPlateTests | PASS |
| Full ViewModelSmokeTests | FAIL: 104 passed, 1 failed |
| App smoke, E2E, QML audit/warning gate | NOT REACHED: canonical script stopped at ViewModel failure |
| Fork branch reachability | BLOCKED: `git ls-remote origin refs/heads/owzx-upstream-2026-07-21` returned no ref |
| Fresh-clone submodule initialization | NOT RUN: parent gitlink cannot be committed before child reachability is proven |

Canonical command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

The isolated failing slot is deterministic:

```text
ViewModelSmokeTests::multiPlateFullStateRoundTrip()
Actual loader.platePrintSequence(2): 0
Expected: 2
tests/ViewModelSmokeTests.cpp:3339
```

## Child Commits

1. `ad3135ce0d` - `fix(260721-jjs): guard null enum key maps`
2. `4cb3b9ce79` - `fix(260721-jjs): support CGAL 5.4 hole refinement`

No parent commit was created.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Resolved Config.hpp replay conflict against current upstream**

- **Found during:** Task 1
- **Issue:** Commit `2976d14778` overlapped an upstream vector-serialization refactor.
- **Fix:** Reapplied only the four documented null-map semantics and replaced the old non-ASCII comments with English ASCII comments.
- **Verification:** Focused test, encoding guard, exact child diff, and `diff --check` pass.
- **Committed in:** `ad3135ce0d`

## Blockers

1. The official upstream baseline changes multi-plate persistence behavior: plate 2 print sequence reloads as `0` instead of `2`. Fixing this requires investigation outside the plan's strict two-file child allowlist.
2. The child branch is intentionally not pushed. Until explicit authorization is provided and `origin` resolves the exact child SHA, the parent gitlink cannot be committed or fresh-clone verified.

## Known Stubs

None introduced.

## Next Task

After the two blockers are resolved, seed the next quick task for viewport context hit and selection routing as recorded in tracker item P12.3.

## Self-Check: PASSED

- Both child commits exist and child HEAD is `4cb3b9ce792f15c38fabfb4bb9700895d32b1166`.
- Every owned root source/document file and this summary exist.
- Summary status is `incomplete`; no parent commit or publication is claimed.
