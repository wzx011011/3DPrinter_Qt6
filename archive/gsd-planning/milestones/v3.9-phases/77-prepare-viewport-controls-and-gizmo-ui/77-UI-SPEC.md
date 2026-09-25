---
phase: 77-prepare-viewport-controls-and-gizmo-ui
status: draft
created: 2026-07-05T20:05:00+08:00
requirements: [VIEWUI-01, GIZMOUI-01]
---

# Phase 77 UI Spec

## Layout Contract

- Main viewport toolbar:
  - Top anchored.
  - Horizontally centered over the viewport workspace.
  - Height 34 px.
  - Icon buttons 30 px or smaller.
  - No text-only action labels.
- Gizmo toolbar:
  - Vertical icon stack on the right side of the bed/workspace area.
  - Width 36 px.
  - Icon buttons 30 px or smaller.
  - Active state uses theme accent; disabled state remains visible but muted.
- View controls:
  - Lower-left compact cluster near the viewport axes/bed controls.
  - No right-top `T/F/R/I/Z` text stack.
- Floating gizmo panels:
  - Use a shared compact panel position below the main toolbar.
  - Use radius 6 or less for operational panels.
  - Do not overlap the bottom plate strip or the right-side gizmo toolbar.

## Behavior Contract

- Gizmo buttons bind to `EditorViewModel::availableGizmoMask`.
- Add plate binds to `EditorViewModel::canAddPlate`.
- Fit/view buttons call the existing `GLViewport` camera APIs.
- Move/rotate/scale panel displays current ViewModel transform values and does
  not own transform math.
- Cut/advanced-cut/wipe-tower related controls keep existing backend calls.
- No dead visible controls: unavailable actions are disabled and expose tooltip
  text through existing `gizmoStatusText`.

## Audit Tokens

The implementation should expose stable source tokens for automated checks:

- `readonly property int viewportToolbarHeight: 34`
- `readonly property int toolbarButtonSize: 30`
- `readonly property int gizmoToolbarWidth: 36`
- `id: viewportActionToolbar`
- `id: viewportGizmoToolbar`
- `id: viewportViewControls`
- `iconSource: iconForTool(toolId)`
- `transformMiniPanel`
- `readonly property int gizmoPanelTopOffset`

## Visual Residuals

Phase 77 can pass with source/build/runtime evidence. Phase 78 must still
capture final Prepare visual evidence against `shotScreen/准备页.png`.
