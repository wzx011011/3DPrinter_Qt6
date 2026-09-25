---
phase: 37-complete-import-and-project-restore
plan: 01
subsystem: import-preview-state
tags: [qt, qml, slicer, import, preview, gcode]
requires:
  - phase: 37
    provides: Phase 37 plan and source-truth context
provides:
  - SliceService result invalidation API for imports/workspace reset
  - PreviewViewModel derived G-code payload reset on slice-result invalidation
  - Consistent import format filters for topbar, Prepare, and Project entry points
  - Regression coverage for import clearing stale slice/Preview/export state
affects: [phase-38, phase-39, phase-40, phase-42]
tech-stack:
  added: []
  patterns: [cpp-owned-state-invalidation, qml-static-entrypoint-audit]
key-files:
  created: []
  modified:
    - src/core/services/SliceService.h
    - src/core/services/SliceService.cpp
    - src/core/viewmodels/EditorViewModel.cpp
    - src/core/viewmodels/PreviewViewModel.h
    - src/core/viewmodels/PreviewViewModel.cpp
    - src/qml_gui/pages/PreparePage.qml
    - src/qml_gui/pages/ProjectPage.qml
    - tests/E2EWorkflowTests.cpp
    - tests/QmlUiAuditTests.cpp
key-decisions:
  - "Slice/Preview/export invalidation is owned by C++ SliceService and ViewModels, not by QML."
  - "Import entry points advertise the same normal model formats: 3MF, STL, OBJ, AMF, STEP, STP."
  - "Preview reset uses a dedicated SliceService signal instead of treating sliceFinished as both result-created and result-cleared."
patterns-established:
  - "Expose resultChanged/sliceResultCleared for downstream viewmodels that cache derived slice data."
  - "Use QML static audit tests to keep file-dialog format contracts aligned across entry points."
requirements-completed: [IMP-01, IMP-02, IMP-03, IMP-04, IMP-05, IMP-06]
duration: 2h 52m
completed: 2026-06-29
---

# Phase 37: Complete Import and Project Restore Summary

**Import now invalidates stale slice output, G-code preview payloads, and export source state while normal import entry points advertise the same model formats.**

## Performance

- **Duration:** 2h 52m
- **Started:** 2026-06-28T15:16:29Z
- **Completed:** 2026-06-29T00:11:25+08:00
- **Tasks:** 4
- **Files modified:** 9 production/test files

## Accomplishments

- Added `SliceService::clearResults()` plus `resultChanged` and `sliceResultCleared` so import/workspace changes clear current G-code output and per-plate slice caches.
- Wired `EditorViewModel::loadFile()`, `clearWorkspace()`, and `refreshAfterLoad()` to invalidate stale slice/Preview/export state.
- Added `PreviewViewModel::resetPreviewState()` so packed GCV1 data, legend, layer/move counters, timings, playback, and tool marker state clear when slice results are invalidated.
- Normalized Prepare and Project model import filters to include `.3mf`, `.stl`, `.obj`, `.amf`, `.step`, `.stp`.
- Added E2E and QML audit coverage for import invalidation and import entry-point format consistency.

## Task Commits

1. **Tasks 1-3: Slice/Preview invalidation and format filters** - `a345d28` (`feat(37-01): invalidate preview on import`)

**Plan metadata:** pending docs closeout commit

## Files Created/Modified

- `src/core/services/SliceService.h` - Added result invalidation API/signals and changed `outputPath` notification to `resultChanged`.
- `src/core/services/SliceService.cpp` - Clears stored result/per-plate state through a public API and notifies Preview consumers.
- `src/core/viewmodels/EditorViewModel.cpp` - Calls result invalidation on import, workspace clear, and project refresh.
- `src/core/viewmodels/PreviewViewModel.h/.cpp` - Adds reusable preview reset logic and listens for slice-result clear events.
- `src/qml_gui/pages/PreparePage.qml` - Adds STEP/STP to normal model-open filters.
- `src/qml_gui/pages/ProjectPage.qml` - Adds AMF to Project page model import filter.
- `tests/E2EWorkflowTests.cpp` - Adds regression test for import clearing stale output and packed preview payload.
- `tests/QmlUiAuditTests.cpp` - Adds static audit for consistent model import formats and routing.

## Decisions Made

- Keep invalidation in C++ because slice output and preview payloads are durable backend state; QML only wires entry-point presentation.
- Do not treat failed path normalization or a rejected import start as a reason to clear old results; clearing happens after import start succeeds.
- Preserve the worker-thread `slicing_` state during cancellation from `clearResults()` so a new slice cannot start until the old worker callback has closed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Replaced invalid hand-written G-code fixture**
- **Found during:** Task 1 verification.
- **Issue:** The first E2E test used a minimal hand-written G-code file with `SliceService::loadGCodeFromPrevious()`, and libslic3r crashed in `GCodeProcessor::update_slice_warnings`.
- **Fix:** Changed the test to generate a real G-code result through the normal STL slice path before re-importing the model.
- **Files modified:** `tests/E2EWorkflowTests.cpp`
- **Verification:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Committed in:** `a345d28`

**2. [Rule 2 - Missing Critical] Avoided reopening the slice gate before worker shutdown**
- **Found during:** Post-implementation code review.
- **Issue:** `clearResults()` initially set `slicing_ = false` while the worker was still cancelling, which could let a new slice start before the previous worker returned.
- **Fix:** Cancellation now signals the worker and clears visible result state, while the worker completion path remains responsible for clearing `slicing_`.
- **Files modified:** `src/core/services/SliceService.cpp`
- **Verification:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Committed in:** `a345d28`

---

**Total deviations:** 2 auto-fixed (1 blocking test fixture, 1 state-machine correctness fix)
**Impact on plan:** Both fixes preserve the planned scope and strengthen correctness.

## Issues Encountered

- A noncanonical local `cmake --build` attempt failed outside the vcvars environment with missing MSVC standard header `utility`. It was not used as verification; the canonical project script was rerun and passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 38 can build on the new invalidation API for prepare readiness and reslice semantics. Phase 40/41 can assume Preview data is cleared when imports replace the source model/project.

---
*Phase: 37-complete-import-and-project-restore*
*Completed: 2026-06-29*
