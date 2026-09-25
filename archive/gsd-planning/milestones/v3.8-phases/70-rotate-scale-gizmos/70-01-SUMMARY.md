---
phase: 70-rotate-scale-gizmos
plan: 01
status: complete
requirements_covered: [GROT-01, GROT-02, GSCA-01, GSCA-02]
files_modified:
  - src/qml_gui/Renderer/RhiViewportRenderer.h
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - src/qml_gui/Renderer/RhiViewport.h
  - src/qml_gui/Renderer/RhiViewport.cpp
  - src/qml_gui/pages/PreparePage.qml
  - src/core/viewmodels/EditorViewModel.h
  - src/core/viewmodels/EditorViewModel.cpp
  - tests/ViewModelSmokeTests.cpp
  - tests/QmlUiAuditTests.cpp
---

# Phase 70 Plan 01 - Summary

**Status:** Complete and verified by the canonical build path.

## What Shipped

### RHI rotate/scale rendering

- `RhiViewportRenderer::uploadGizmoBuffer()` now uploads one static gizmo
  buffer containing move, rotate, and scale geometry from `GizmoGeometry`.
- Stored adjusted `GizmoGeometryOffsets` for move, rotate, and scale so draw
  calls follow the Phase 66 builder metadata instead of hardcoded
  rotate/scale offsets.
- Added `renderRotateGizmo()` to draw three torus rings with the existing
  no-depth-write triangle gizmo pipeline when `gizmoMode == GizmoRotate`.
- Added `renderScaleGizmo()` to draw three axis shafts with the line pipeline
  and three box handles with the triangle pipeline when
  `gizmoMode == GizmoScale`.

### RhiViewport rotate/scale interaction

- Generalized the Phase 69 drag state with an active drag mode, axis, center,
  ray-axis parameter, and rotate-start angle.
- `pickGizmoAxisAt()` now dispatches to `GizmoMath::pickMoveAxis`,
  `pickRotateAxis`, or `pickScaleAxis` according to `gizmoMode`.
- Rotate drags use `GizmoMath::computeRotateAngle()` and emit per-frame
  `gizmoRotateRequested(axis, radians)` deltas.
- Scale drags use `GizmoMath::rayToAxisT()` and emit per-frame
  `gizmoScaleRequested(axis, factor)` deltas with the repository's extracted
  GLViewport scale factor semantics.
- Active gizmo drags continue to accept mouse events, so camera orbit is not
  triggered during rotate or scale drags.

### EditorViewModel drag application and undo coalescing

- Added QML-invokable rotate drag lifecycle:
  `beginGizmoRotateDrag()`, `applyGizmoRotateDelta(axis, radians)`, and
  `endGizmoRotateDrag()`.
- Added QML-invokable scale drag lifecycle:
  `beginGizmoScaleDrag()`, `applyGizmoScaleFactor(axis, factor)`, and
  `endGizmoScaleDrag()`.
- Rotate deltas are converted from radians to degrees at the viewmodel
  boundary, matching `ProjectServiceMock::objectRotation()` storage.
- Scale factors are finite-checked, clamped to `0.01`, and applied only on
  the picked axis.
- Each drag applies frame updates immediately but records one independent
  `TransformCommand` at drag end.

### PreparePage bridge

- Added `onGizmoRotateRequested` and `onGizmoScaleRequested` forwarding.
- `onGizmoDragBegin` and `onGizmoDragEnd` now route to move, rotate, or scale
  viewmodel lifecycle methods based on the drag mode captured at begin.
- QML remains a thin bridge; all picking, ray, angle, scale-factor, and
  transform logic stays in C++.

### Test coverage

- Added `ViewModelSmokeTests::gizmoRotateDragCoalescesIntoSingleUndoCommand()`.
- Added `ViewModelSmokeTests::gizmoScaleDragCoalescesIntoSingleUndoCommand()`.
- Added `QmlUiAuditTests::rhiRotateScaleGizmoBridgeStaysCppOwned()`.
- RED verification was captured before implementation: focused build failed
  because `EditorViewModel` did not yet expose the new rotate/scale methods.

## Source-Truth Alignment

- GROT-01 is covered by RHI rotate torus rendering and rotate drag applying to
  the selected object's axis rotation.
- GROT-02 is covered by `GizmoMath::pickRotateAxis()` and
  `computeRotateAngle()` ownership in `RhiViewport`.
- GSCA-01 is covered by RHI scale shaft/box rendering and axis-only scale drag
  applying to the selected object.
- GSCA-02 is covered by `GizmoMath::pickScaleAxis()` and `rayToAxisT()`
  ownership in `RhiViewport`.

## Carry-Forward

- Full OrcaSlicer `GLGizmoScale3D` ten-grabber plane-ratio behavior remains a
  future source-truth improvement. Phase 70 deliberately follows the
  repository's extracted GLViewport helper semantics for the v3.8 RHI parity
  slice.
- Hover/highlight colors remain deferred.
- Manual visual UAT for all gizmo modes remains part of Phase 73's final
  running-app verification pass.
