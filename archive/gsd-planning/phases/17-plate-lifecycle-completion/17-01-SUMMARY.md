---
phase: 17-plate-lifecycle-completion
plan: 01
subsystem: feature
tags: [partplate, lifecycle, clone, reorder, printable, qt6, qml, slice-all]

requires:
  - phase: 16
    provides: "PartPlate + PartPlateList domain model + ProjectServiceMock re-backed"
provides:
  - "clonePlate (deep copy incl. ModelObjects, D-06)"
  - "movePlate (metadata reorder + reindex, D-07)"
  - "setPlatePrintable / isPlatePrintable (D-08)"
  - "slice-all queue excludes non-printable plates"
  - "5 QML context-menu items wired to editorVm"
  - "3 deterministic tests"
affects: [18-3mf-multi-plate-persistence, 19-per-plate-slice-scheduling, 20-verification-and-handoff]

key-files:
  created: []
  modified:
    - src/core/model/PartPlateList.h
    - src/core/model/PartPlateList.cpp
    - src/core/services/ProjectServiceMock.h
    - src/core/services/ProjectServiceMock.cpp
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/qml_gui/pages/PreparePage.qml
    - tests/ViewModelSmokeTests.cpp

key-decisions:
  - "D-06 clonePlate deep-copies ModelObjects via duplicateObject + explicit membership assignment"
  - "D-07 movePlate is metadata-only (geometry recompute deferred)"
  - "D-08 per-plate printable flag + slice-all queue filter"
  - "D-09 QML context-menu integration (CxMenuItem) over new buttons"

patterns-established:
  - "PartPlateList::movePlate current-plate tracking through shift ranges"
  - "clonePlate descending-iteration to survive duplicateObject's index shifts"

requirements-completed: [PLATE-03, PLATE-04, PLATE-05]

duration: 35min
completed: 2026-06-25
---

# Plan 17-01: Plate Lifecycle Completion Summary

**Three missing plate lifecycle operations (clone/reorder/printable) added end-to-end with QML UI and tests — all built on the Phase 16 model, no storage refactor.**

## Performance
- **Duration:** ~35 min (1 build-fix cycle)
- **Files modified:** 8
- **Net LOC:** +296 / -1

## Accomplishments
- **clonePlate (PLATE-03, D-06):** deep copy of plate metadata + ModelObjects. Reuses `duplicateObject` for the object copy; assigns new object indices to the destination plate. MAX_PLATE_COUNT=36 guard.
- **movePlate (PLATE-04, D-07):** `PartPlateList::movePlate` — vector shift + reindex + current-plate tracking through the shifted range. Pure metadata (no geometry recompute — deferred).
- **per-plate printable (PLATE-05, D-08):** `setPlatePrintable`/`isPlatePrintable`; `EditorViewModel::requestSliceAll` queue filter now excludes both locked AND non-printable plates.
- **QML UI (D-09):** 5 new context-menu items in PreparePage plate menu — printable toggle, clone, move-left, move-right. All wired to `editorVm`, all `qsTr` strings, no empty handlers.

## Task Commits
1. **All tasks (T1-T6):** `62d9622` (feat) — single commit; the phase is additive and cohesive.

## Decisions Made
- **clonePlate membership:** `duplicateObject`'s HAS_LIBSLIC3R branch copies the object + metadata but does NOT assign plate membership. clonePlate explicitly calls `dst->addInstance(dupIdx, 0)` after each duplicate. (Build-fix #1 surfaced this.)
- **Descending iteration in clonePlate:** duplicateObject inserts at sourceIndex+1, shifting higher indices — iterate source indices descending so unprocessed lower indices stay valid (mirrors deleteObject's reverse-order pattern).

## Deviations from Plan
- Plan suggested clonePlate switch the current plate temporarily; the executed version skips that (since duplicateObject's HAS branch doesn't use current plate) and instead explicitly assigns membership. Cleaner.

## Issues Encountered
- **Build-fix #1:** clone test failed (`plateObjectCount(1) == 0`) because duplicateObject's HAS branch doesn't set plate membership. Fixed by explicit `dst->addInstance(dupIdx, 0)`.

## Next Phase Readiness
- **Phase 17 complete.** All three lifecycle ops work end-to-end. Ready for Phase 18 (3MF persistence), which will persist this new state (printable, clone results, order) to 3MF via store_to_3mf_structure.
- clonePlate's geometry-offset translation still deferred (no plate origins yet) — recorded, Phase 18+ when bed geometry lands.

---
*Phase: 17-plate-lifecycle-completion*
*Completed: 2026-06-25*
