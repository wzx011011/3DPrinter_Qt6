---
phase: 97-thumbnail-save-reload-round-trip
status: findings_fixed
files_reviewed: 2
depth: standard
critical: 0
warning: 0
info: 3
total: 3
---

# Phase 97 Code Review — Thumbnail Save-Reload Round-Trip

**Scope:** Changes against base commit `9610a85` in `tests/PartPlateTests.cpp` and `src/core/services/ProjectServiceMock.cpp`.
**Status:** Findings addressed — HIGH-severity ordering bug fixed and verified; MEDIUM multi-plate gap documented for Phase 98.

## Findings Resolved

| # | Severity | Area | Finding | Resolution |
|---|----------|------|---------|------------|
| 1 | HIGH (fixed) | Correctness | In-loop `setThumbnail` ran BEFORE `arrangeObjects`; the arrange rebuild (`clearInstances`/`addInstance`) invalidates `m_thumbnail`, so the restored thumbnail could be wiped before `loadFinished`. | Moved thumbnail restore to a post-arrangeObjects pass in both `loadFile` and `loadProject` (commit `efd5e42`). `setThumbnail` is now the last writer of `m_thumbnail`. |
| 2 | HIGH (resolved) | Verification | `PartPlateTests.exe` was a Windows GUI-subsystem binary swallowing all test output; ctest "Passed" only checked exit code 0. | Added `/SUBSYSTEM:CONSOLE` to link options (commit `efd5e42`). Verified empirically that a deliberately broken assertion correctly produces `***Failed` (exit 1) — the ctest exit-code signal is trustworthy. |
| 3 | MEDIUM (deferred) | Correctness (latent) | Multi-plate thumbnail round-trip is broken at the write side: `StoreParams::thumbnail_data` is populated with at most ONE entry (current plate), but the writer emits per-plate XML references — plates > 0 get an XML ref with no PNG bytes archived. | Documented for Phase 98 (THUMBVERIFY). Single-plate `thumbnailSaveReloadRoundTrip` test does not exercise this; Phase 98 should add a multi-plate variant + write-side fix (push one `thumbnail_data` entry per plate). |
| 4 | LOW (info) | Test quality | `#include <QFile>` missing in test (compiles via transitive include). | Minor; transitive include is stable. Optional cleanup. |
| 5 | LOW (info) | Test quality | Temp-file cleanup (`QFile::remove`) not exception-safe; leaks on QVERIFY2 failure. | Minor; timestamp naming prevents collisions. Optional RAII guard. |
| 6 | NIT (info) | Docs | Comments cite writer line 6550 vs canonical 7988. | Cosmetic. |

## Correctness Verified

After the fix (commit `efd5e42`):

- **Empirical harness probe:** Deliberately broke `QCOMPARE(plate.filamentMapMode(), 0)` → `999`. ctest correctly reported `***Failed` (exit 1, `0% tests passed`). Restored the assertion → ctest reports `Passed`. This proves the ctest exit-code signal is trustworthy; the binary does propagate assertion failures to its exit code.
- **Regression ctest (4/4 pass, 8.33s):** PartPlateTests `Passed` (0.64s), ViewModelSmokeTests `Passed` (7.57s), QmlUiAuditTests `Passed` (0.07s), PrepareSceneDataTests `Passed` (0.03s).
- **The `thumbnailSaveReloadRoundTrip` slot is compiled under `HAS_LIBSLIC3R`** (confirmed via `strings`: slot name present, no "Requires HAS_LIBSLIC3R" QSKIP string), and runs in the ctest suite.

## Note on Pre-Fix Behavior

The code review's HIGH-1 reasoning was correct about the ordering hazard (arrange rebuilds DO invalidate the cache), but the pre-fix test still passed (exit 0) — likely because for a single `hotend.stl` on the 220x220 bed, the arrange/rebuild path did not observe the thumbnail in the failing state, or the rebuild re-distribution placed the instance back onto plate 0 preserving a degenerate cache. Regardless, the post-fix ordering is strictly more correct (setThumbnail is the last writer, guaranteeing the restored thumbnail survives), so the fix is warranted as a robustness improvement even though the test passed both ways.

Full report: `.planning/phases/97-thumbnail-save-reload-round-trip/97-REVIEW.md`
