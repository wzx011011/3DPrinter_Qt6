# Phase 69: Move Gizmo Pick + Drag Interaction - Context

**Gathered:** 2026-07-04
**Mode:** Auto-generated (interaction phase)

<domain>
Close the interaction loop for the move gizmo on the D3D11 RHI path: user
clicks an axis arrow → drags → the selected object translates along that axis.

Deliverables:
- RhiViewport gains gizmo-axis state: `m_gizmoAxis` (0/1/2/0=none),
  `m_gizmoDragging` (bool), `m_gizmoDragStartT` (float, ray-axis param at
  press), `m_gizmoDragStartCenter` (QVector3D).
- New `pickGizmoAxisAt(QPointF)` helper on RhiViewport using
  `GizmoMath::pickMoveAxis` (Phase 65) with the camera matrices.
- mousePressEvent: when `gizmoMode == Move` and an object is selected, call
  `pickGizmoAxisAt`; if hit, set `m_gizmoDragging=true`, record start T via
  `GizmoMath::rayToAxisT`, and consume the event (no camera orbit).
- mouseMoveEvent: when `m_gizmoDragging`, compute new T, delta = (newT -
  startT) * axisDir * gizmoScale, emit `gizmoMoveRequested(delta)`.
- mouseReleaseEvent: end drag.
- New signal `gizmoMoveRequested(QVector3D worldDelta)`.
- EditorViewModel: new Q_INVOKABLE `applyGizmoMoveDelta(QVector3D delta)` that
  translates the selected object by delta (single TransformCommand at drag
  end, not per-frame spam).
- PreparePage.qml: bind `onGizmoMoveRequested` → `editorVm.applyGizmoMoveDelta`.

Out of scope: rotate/scale drag (Phase 70), precise object picking (Phase 72).
</domain>

<decisions>
- **Single drag→undo entry**: drag emits per-frame deltas but EditorViewModel
  accumulates them; on drag-end (or each frame) writes the cumulative delta
  via one TransformCommand. Simplest: emit only on release (renderer tracks
  cumulative delta internally, emits once). Chosen: emit per-frame deltas,
  ViewModel uses a "drag in progress" flag to coalesce — final delta becomes
  one undo entry on release.
- **gizmoScale reuse**: the same `max(dist*0.15, 5.0)` formula from
  RhiViewportRenderer (Phase 68) is computed in RhiViewport for pick
  thresholding.
- **Camera consume**: when gizmoDragging, mouseMove/Release fully consume the
  event so camera orbit never fires mid-drag.
</decisions>
