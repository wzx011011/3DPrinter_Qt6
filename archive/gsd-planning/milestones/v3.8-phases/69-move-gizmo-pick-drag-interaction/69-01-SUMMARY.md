---
phase: 69-move-gizmo-pick-drag-interaction
plan: 01
status: complete
requirements_covered: [GMOV-03, GMOV-04]
files_modified:
  - src/qml_gui/Renderer/RhiViewport.h
  - src/qml_gui/Renderer/RhiViewport.cpp
  - src/qml_gui/pages/PreparePage.qml
  - src/core/viewmodels/EditorViewModel.h
  - src/core/viewmodels/EditorViewModel.cpp
  - tests/ViewModelSmokeTests.cpp
  - tests/QmlUiAuditTests.cpp
---

# Phase 69 Plan 01 - Summary

**Status:** Complete and verified by the canonical build path.

## What Shipped

### RhiViewport move-axis picking and drag

- Added move-gizmo drag state to `RhiViewport`: picked axis, active drag flag,
  ray-axis start parameter, and the drag-center snapshot.
- Added `pickGizmoAxisAt()` using `GizmoMath::pickMoveAxis()` with the RHI
  camera matrices and `GizmoCenter::fromSelectedBatch()`.
- `mousePressEvent()` now prioritizes move-axis hit testing over object picking
  when Move mode is active and an object is selected.
- `mouseMoveEvent()` computes per-frame world deltas through
  `GizmoMath::computeRay()` and `GizmoMath::rayToAxisT()`, emits
  `gizmoMoveRequested(delta)`, and consumes the event so camera orbit does not
  run during gizmo drag.
- `mouseReleaseEvent()` ends the drag and emits `gizmoDragEnd()`.
- Defensive cleanup resets stale drag state on a new press and emits a balanced
  drag-end signal if an invalid active axis is ever detected.

### EditorViewModel drag application and undo coalescing

- Added QML-invokable drag lifecycle methods:
  `beginGizmoMoveDrag()`, `applyGizmoMoveDelta(dx, dy, dz)`, and
  `endGizmoMoveDrag()`.
- Per-frame drag deltas update the selected object's position immediately,
  invalidate stale slice results, and emit `stateChanged()`.
- On drag end, one `TransformCommand` records the whole drag. When no macro is
  already open, the command is wrapped in a one-command macro so it cannot
  merge with a previous numeric transform command for the same object.

### PreparePage bridge

- Wired `GLViewport`/RHI signals to the viewmodel:
  `onGizmoDragBegin`, `onGizmoMoveRequested`, and `onGizmoDragEnd`.
- QML remains a thin bridge; all picking, ray math, axis delta calculation, and
  undo behavior stay in C++.

### Test coverage

- Added `ViewModelSmokeTests::gizmoMoveDragCoalescesIntoSingleUndoCommand()`.
  It verifies frame deltas move the object, one drag produces one undo entry,
  the drag does not merge into a previous transform, and undo/redo restore the
  expected positions.
- Added `QmlUiAuditTests::rhiMoveGizmoDragBridgeStaysCppOwned()` to guard the
  RHI signal bridge and prevent moving gizmo math into QML.
- Updated two existing QML audit assertions that had become stale after earlier
  phases:
  - Dirty flags are still consumed only after scene upload succeeds, but Phase
    68's gizmo upload may now occur before `takeDirtyFlags()`.
  - The renderer vertex layout now lives in shared `GizmoVertex.h`, with
    `RhiViewportRenderer::Vertex` as an alias.

## Source-Truth Alignment

- GMOV-03 is covered by RHI mouse press hit testing through the extracted
  `GizmoMath::pickMoveAxis()` helper.
- GMOV-04 is covered by per-axis drag deltas, C++ object translation, consumed
  mouse events during active drag, and single undo-step coalescing.

## Carry-Forward

- Phase 70 can reuse the same signal pattern and viewmodel lifecycle approach
  for rotate and scale, but should add separate command semantics for rotation
  and scale rather than overloading move deltas.
- Full visual confirmation of rendered axis handles remains part of the v3.8
  end-to-end verification pass, but Phase 69 itself passed automated build,
  smoke, static audit, app launch, and E2E checks.
