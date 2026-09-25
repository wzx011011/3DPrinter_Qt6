---
phase: 25-prepare-model-mesh-rendering-and-camera-interaction
plan: 03
subsystem: rendering
tags: [qrhi, prepare, selection, hover, picking, performance, tdd]

requires:
  - phase: 25
    plan: 02
    provides: QRhi active-plate model buffer rendering and camera uniforms
provides:
  - Source-index QRhi picking bridge to EditorViewModel selection
  - C++ hover tracking through projected model batch bounds
  - Independent selected/hovered highlight buffer path
affects: [phase-25, rhi-viewport, editor-viewmodel, prepare-page]

tech-stack:
  added: []
  patterns:
    - GUI-thread picking state in RhiViewport
    - QML signal-forwarding only for picked source objects
    - Selection/hover highlight upload separated from full model buffer upload

key-files:
  created: []
  modified:
    - tests/ViewModelSmokeTests.cpp
    - tests/QmlUiAuditTests.cpp
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - src/qml_gui/Renderer/GLViewport.h
    - src/qml_gui/Renderer/GLViewport.cpp
    - src/qml_gui/Renderer/SoftwareViewport.h
    - src/qml_gui/Renderer/SoftwareViewport.cpp
    - src/qml_gui/pages/PreparePage.qml

key-decisions:
  - "EditorViewModel remains the selection source of truth; QRhi emits source-object pick signals only."
  - "RhiViewport performs projected bounds picking in C++ using CameraController matrices and active model batch metadata."
  - "Selected/hovered feedback uses a dedicated highlight vertex buffer so DirtySelection does not reupload the full model buffer."

patterns-established:
  - "New PreparePage viewport signals require GL/Software fallback type parity."
  - "Renderer selection APIs use active-plate source object indices, not visible-list indices."

requirements-completed: [PREP-03, PREP-04, PREP-07]

duration: 36min
completed: 2026-06-27
---

# Phase 25 Plan 03: Selection, Hover, And Picking Bridge Summary

**QRhi model picking now selects EditorViewModel source objects and renders hover/selection feedback without full mesh reupload**

## Accomplishments

- Added RED smoke/audit tests for source-index selection, QML forwarding-only behavior, fallback viewport parity, C++ picking ownership, and no full model buffer reupload on selection.
- Added `EditorViewModel::selectSourceObject(int sourceIndex)` and kept selection mutation in the viewmodel.
- Added `RhiViewport::objectPickedSource(int sourceIndex)` and `hoveredSourceObjectIndex` parity across QRhi, GL, and Software viewport types.
- Implemented QRhi C++ picking from a GUI-thread `PrepareSceneData` snapshot using projected model batch bounds and current camera MVP.
- Forwarded picked source objects in `PreparePage.qml` with no QML-side mapping, filtering, parsing, or geometry logic.
- Added a dedicated QRhi highlight vertex buffer for selected and hovered batches, leaving the resident model buffer untouched for `DirtySelection`.

## Task Commits

1. **Task 25-03-T1: Add picking selection bridge tests first** - `9c4b4ee` (test)
2. **Tasks 25-03-T2/T3: Implement source selection bridge, C++ picking, and highlight buffer** - `e756073` (feat)

## Verification

- RED evidence: `build/phase25_03_red.log` failed at `ViewModelSmokeTests.cpp` because `EditorViewModel::selectSourceObject` did not exist.
- GREEN evidence: `build/phase25_03_green_attempt2.log` completed successfully.
- Canonical verification command:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Selection bridge validates active rendered plate membership**
- **Found during:** Task 25-03-T2 implementation review
- **Issue:** Visible object list fallback can show all objects on an empty plate, but QRhi renders only `activePlateObjectIndices`. Using visible indices for source selection could select a non-rendered object.
- **Fix:** `selectSourceObject()` validates against `ProjectServiceMock::currentPlateObjectIndices()`.
- **Files modified:** `src/core/viewmodels/EditorViewModel.cpp`, `tests/ViewModelSmokeTests.cpp`
- **Verification:** `build/phase25_03_green_attempt2.log` passed.
- **Committed in:** `e756073`

---

**2. [Rule 2 - Performance Correctness] Corrected audit boundary for model upload helper**
- **Found during:** Static audit review
- **Issue:** The audit originally inspected from `uploadModelBuffer` through `uploadCameraUniform`, which would include the new `uploadHighlightBuffer` function and produce a false positive.
- **Fix:** Audit now checks only the `uploadModelBuffer` function body up to `uploadHighlightBuffer`.
- **Files modified:** `tests/QmlUiAuditTests.cpp`
- **Verification:** `build/phase25_03_green_attempt2.log` passed.
- **Committed in:** `9c4b4ee`

---

**Total deviations:** 2 auto-fixed (one correctness guard, one audit precision fix)
**Impact on plan:** The implementation remains within the planned selection/hover bridge scope.

## User Setup Required

None. QRhi remains behind the existing `OWZX_RHI_RENDERER` gate.

## Next Phase Readiness

Plan 25-04 can close Phase 25 with verification traceability, requirement mapping, and handoff. The remaining work should document the QRhi Prepare mesh/camera/selection pipeline and confirm fallback behavior remains clean.

## Self-Check: PASSED

- Plan task commits exist: `9c4b4ee`, `e756073`.
- Canonical verification passed after final implementation.
- QML remains forwarding-only for renderer picks.
- `DirtySelection` is audited not to reupload the full model buffer.

---
*Phase: 25-prepare-model-mesh-rendering-and-camera-interaction*
*Completed: 2026-06-27*
