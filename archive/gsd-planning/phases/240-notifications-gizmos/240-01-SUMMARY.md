# Phase 240 Summary — Notification Stacking And Gizmo Interaction Depth

**Completed:** 2026-08-16
**Requirements:** NOTI-01, GIZ-01..06 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (agent run +
independent re-run before commit; all 8 suites, app launch, E2E).

## Changes (19 files, +3723/−529)

1. **NOTI-01 stacked notifications**: BackendContext's single-slot queue is
   now a `notificationStack` QVariantList (index 0 = most important, visible
   cap 6 + overflow queue) with upstream semantics — duplicate compression
   (activate_existing, NotificationManager.cpp:2643-2675) with repeatCount
   escalation, stable importance sort mapped onto the upstream 9-level
   NotificationLevel ladder (NotificationManager.hpp:158-180), per-entry
   dismissal (dismissNotificationById), history ring (100) preserved; every
   existing entry point (postError/postNotification/progress/hints/export/
   arrange/confirm/cancel) rerouted; legacy lastError getters keep last-post
   wins. ErrorToast.qml rewritten as a stacked ToastContainer (Repeater +
   per-entry auto-dismiss/close/hover-pause/persistent-flip, xN badge).
2. **GIZ-01 numeric transform input**: the read-only transform mini panel
   now uses editable TransformMetricField components committing on
   editingFinished through the undoable setters (TransformCommand).
   World/local toggle honestly gated: single instance-space transform API
   cannot represent the distinction (documented; upstream
   GizmoObjectManipulation ECoordinatesType).
3. **GIZ-02 painter smart fill + filters**: Shift+click smart fill via
   upstream seed_fill_select_triangles + seed_fill_apply_on_triangles
   (two-stage raycast, shared PaintCommand undo), angle-threshold slider
   (select_patch highlight_by_angle_deg, GLGizmoPainterBase.cpp:800-805),
   overhang-only filter toggle.
4. **GIZ-03 Flatten face pick**: hover facet highlight (translucent fill,
   upstream FLATTEN hover color) + click-to-place rotating the picked world
   normal to −Z (Selection::flattening_rotate math, one TransformCommand,
   offset preserved); flattenRotationForNormal pure helper tested on 5
   normals.
5. **GIZ-04 Cut plane 3D drag/rotate**: plane-body drag along the axis +
   two tilt grabbers (renderer hit-testing, sqrt(2) plane expansion);
   cutRotation/advCutRotation properties; rotated cuts execute through
   Cut::perform_with_plane (0/0/0 reproduces the axis-aligned cut);
   AdvancedCut mirrors drags into advCut state.
6. **GIZ-05 Measure 3D overlay**: in-scene dimension lines + cross anchors
   (line pipeline, GizmoMeasure gate) + numeric labels as QML Text projected
   from 3D anchors (projectWorldToScreen; RHI has no text pipeline — the
   plan-allowed 2D projection form). AssemblePage overlay precedent reused.
7. **GIZ-06 Emboss in-place + Simplify preview + SVG workbench**: emboss
   panel populates from the selected text volume's TextConfiguration
   (volumeTextConfiguration) and re-generates in place (updateTextVolume:
   text2shapes/polygons2model, set_mesh, name+config update, new
   UpdateVolumeMeshCommand undo); simplify three-stage Preview→Apply/Cancel
   (QtConcurrent decimation on a copy — count shown, model untouched until
   Apply); SVG drops resolve the bed-plane point and place via
   placeSvgAtBedPoint with the existing transform panel.

## Tests

- ViewModelSmokeTests +5 slots (150 green): notification stack ordering/
  compression/dismissal + legacy getters; smart fill angle + overhang
  filter on a bent synthetic mesh; flatten rotation composition property
  (5 normals → −Z); emboss in-place regen (config round-trip, no new
  volume, mesh changed); simplify preview decimates without mutation.
- QmlUiAuditTests +gizmoDepthAndNotificationSourceAudit (147 green): 40
  source locks across all 7 work items.

## Honest gates / upstream deltas (documented in code)

- GIZ-01: single-space panel (API cannot represent world/local).
- GIZ-02: overhang threshold is a VM property, not per-object
  support_threshold_angle config (plumbing doesn't reach the paint gizmo);
  smart fill fires on click/press — shift-drag keeps the erase-brush path.
- GIZ-03: hover highlights the raw facet (not upstream's convex-hull plane
  clusters); click/undo/offset are full ports.
- GIZ-04: minimal grabber set (plane drag + two tilt grabbers); upstream's
  X/Y-move arrows + Z-rotation ring not ported.
- GIZ-05: numeric labels are projected 2D Text (plan-allowed).
- GIZ-06: SVG drop approximates the ray hit with the Z=0 bed intersection;
  emboss populate happens on entering the gizmo (re-select while open
  doesn't re-populate, protecting in-progress edits).
