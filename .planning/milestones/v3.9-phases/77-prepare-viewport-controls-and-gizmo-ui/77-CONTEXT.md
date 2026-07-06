---
phase: 77-prepare-viewport-controls-and-gizmo-ui
status: active
created: 2026-07-05T20:05:00+08:00
requirements: [VIEWUI-01, GIZMOUI-01]
depends_on: [74-prepare-source-truth-gap-audit, 76-prepare-workflow-panels-restoration]
---

# Phase 77 Context: Prepare Viewport Controls And Gizmo UI

## Source-Truth Inputs

- Visual truth: `shotScreen/准备页.png`.
- Gap rows:
  - `PREP-VIEWPORT`
  - `PREP-VTOOLBAR`
  - `PREP-GIZMOFLOAT`
  - `PREP-VIEWOPTS`
- Upstream anchors:
  - `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp`
    toolbar/view/camera placement and `zoom_to_*` behavior.
  - `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosManager.*`
    gizmo availability, hover/active state, and icon toolbar model.
  - `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmo*.cpp`
    gizmo input-window behavior.

## Current Qt Targets

- `src/qml_gui/components/GLToolbars.qml`
  - Owns Prepare viewport action toolbar, gizmo toolbar, and view preset
    controls.
  - Still uses text labels such as `M`, `R`, `S`, `T`, `F`, `P+`, `AC`, and
    `SVG`.
- `src/qml_gui/pages/PreparePage.qml`
  - Owns `GLViewport` placement and existing QML gizmo floating panels.
  - Already wires RHI move/rotate/scale drag requests into `EditorViewModel`.
- `src/qml_gui/Renderer/RhiViewport.*`
  - Owns camera preset/fit behavior and RHI gizmo rendering. Phase 77 should
    not rewrite this path unless a UI binding requires it.
- `tests/QmlUiAuditTests.cpp`
  - Existing source audits cover backend gates and RHI gizmo ownership.

## Known UI Deviations

1. The viewport action toolbar is a dark rounded block at the top-left; the
   target uses a compact icon row near the top center of the viewport.
2. The gizmo toolbar is a tall left-edge text bar; the target shows a compact
   vertical icon stack near the bed/right workspace.
3. View controls are a right-top text stack; the target uses small viewport
   affordances near the lower-left bed/camera area.
4. Move/rotate/scale states lack a compact visible transform panel even though
   the RHI handles are functional.
5. Advanced floating panels share large top-center blocks that can collide with
   toolbar and bed focus.

## Phase Boundary

In scope:
- Replace text-heavy toolbar buttons with icon-first controls using existing
  QML controls and local SVG assets.
- Reposition toolbar groups to match the screenshot hierarchy.
- Add lightweight, state-only transform panels for move/rotate/scale.
- Reposition existing gizmo panels so they do not overlap core viewport
  controls.
- Preserve all `EditorViewModel` gates and RHI drag behavior.

Out of scope:
- New slicing, arranging, or gizmo behavior not already exposed by the backend.
- Rewriting RHI renderer geometry, picking, or camera math.
- Full pixel screenshot acceptance; Phase 78 owns final visual evidence.
