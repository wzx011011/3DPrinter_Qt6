---
phase: 38-prepare-readiness-and-slice-invalidation
plan: 01
subsystem: prepare-readiness
tags: [qt, qml, slicer, readiness, gcode, preview]
requires:
  - phase: 37-complete-import-and-project-restore
    provides: import-triggered slice and preview invalidation
provides:
  - C++ current-plate readiness API for slice, Preview, and G-code export
  - per-plate slice result status with missing, valid, and stale states
  - centralized invalidation helpers for representative Prepare mutations
  - Prepare UI bindings for backend readiness and disabled reasons
affects: [phase-39-slicing-state-machine, phase-40-preview-data, phase-42-gcode-export]
tech-stack:
  added: []
  patterns:
    - C++ viewmodel owns durable readiness decisions; QML only binds availability and hints
key-files:
  created:
    - .planning/phases/38-prepare-readiness-and-slice-invalidation/38-REVIEW.md
  modified:
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/core/services/SliceService.cpp
    - src/qml_gui/pages/PreparePage.qml
    - src/qml_gui/panels/SliceProgress.qml
    - tests/ViewModelSmokeTests.cpp
    - tests/E2EWorkflowTests.cpp
    - tests/QmlUiAuditTests.cpp
key-decisions:
  - "Readiness and stale-result decisions live in EditorViewModel/SliceService, not QML."
  - "Invalidating the current result plate removes SliceService metadata and clears the current output path."
  - "Prepare UI renders valid/stale/missing plate status through plateSliceResultStatus(index)."
patterns-established:
  - "Use canPreview/canExportGCode/action hints for Prepare action gating."
  - "Use per-plate invalidation where the affected plate is known; use all-plate invalidation for global bed/arrange/index-changing operations."
requirements-completed: [PREP-01, PREP-02, PREP-03, PREP-04]
duration: 3h 19m
completed: 2026-06-29
---

# Phase 38: Prepare Readiness and Slice Invalidation Summary

**Prepare now has a C++-owned validity contract for slicing, Preview, export, and stale per-plate G-code results.**

## Performance

- **Duration:** 3h 19m
- **Started:** 2026-06-28T16:26:14Z
- **Completed:** 2026-06-29T01:45:45+08:00
- **Tasks:** 5
- **Files modified:** 9

## Accomplishments

- Added `EditorViewModel` readiness API: `canPreview`, `canExportGCode`, `sliceReadinessReason`, `previewActionHint`, `exportActionHint`, and `plateSliceResultStatus(index)`.
- Added missing/valid/stale per-plate result tracking and invalidation helpers for object, plate, transform, arrange, bed, printable, and sequence/config-style mutations.
- Hardened `SliceService::removePlateResult()` so invalidating the current result plate clears stale `outputPath`.
- Bound Prepare export, plate cards, and SliceProgress actions to backend availability and reasons.
- Added RED-first ViewModel/E2E tests and QML audit coverage for the new readiness contract.

## Task Commits

1. **Task 1: RED readiness/stale tests** - `5affe50` (`test(38-01)`)
2. **Tasks 2-3: C++ readiness and invalidation model** - `b8144eb` (`feat(38-01)`)
3. **Task 4: Prepare UI bindings and QML audit** - `92c83e0` (`feat(38-01)`)

## Files Created/Modified

- `src/core/viewmodels/EditorViewModel.h` - exposes readiness properties, result status enum, and invalidation helpers.
- `src/core/viewmodels/EditorViewModel.cpp` - implements readiness, stale tracking, action gating, and mutation invalidation.
- `src/core/services/SliceService.cpp` - removes stale current output paths when plate results are invalidated.
- `src/qml_gui/pages/PreparePage.qml` - guards export dialog and renders plate status dots through backend status.
- `src/qml_gui/panels/SliceProgress.qml` - binds Preview/export buttons and hints to backend availability.
- `tests/ViewModelSmokeTests.cpp` - covers missing-result readiness and disabled reasons.
- `tests/E2EWorkflowTests.cpp` - covers real-slice result validity and bed-change stale invalidation.
- `tests/QmlUiAuditTests.cpp` - guards QML readiness bindings.
- `.planning/phases/38-prepare-readiness-and-slice-invalidation/38-REVIEW.md` - code review report.

## Decisions Made

- Kept `isPlateSliced()` as compatibility shorthand, but backed user-facing status with `plateSliceResultStatus(index)`.
- Removed `SliceService`'s automatic global `projectChanged -> clearResults()` behavior so scoped invalidation can preserve known per-plate state.
- Used all-plate invalidation for bed shape, arrange-all, delete/clone/move plate, and other global or index-changing operations.

## Deviations from Plan

- `SliceService.h` did not need changes; the existing public `removePlateResult()` method was sufficient after implementation was fixed.
- SliceProgress still contains an old compatibility text based on `isPlateSliced(index)`, but explicit status background/border and Prepare plate dots use `plateSliceResultStatus(index)`.

## Issues Encountered

- The first GREEN verification run timed out after 10 minutes while still building, leaving orphan `ninja/cmake` processes. Those orphan build processes were stopped and the canonical verification was rerun successfully with a longer timeout.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 39 can build on the readiness model for the full slicing/reslicing state machine. Preview/export entry points now refuse stale or missing current-plate results before the deeper G-code preview and export finalization work begins.

---
*Phase: 38-prepare-readiness-and-slice-invalidation*
*Completed: 2026-06-29*
