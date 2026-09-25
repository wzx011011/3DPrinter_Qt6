# Phase 82 UI Spec: Preview Roles And Color Modes

## Scope

| Surface | Contract |
|---|---|
| View mode combo | Keep the 17 upstream view modes in order; selecting a mode calls `PreviewViewModel::setViewModeIndex`. |
| Availability status | If the selected mode lacks real data in the current Qt path, show a compact non-blocking status pill near the combo. |
| Role visibility | Role rows show upstream-mapped swatches and call `toggleRoleVisibility(roleIndex)`. |
| Legend | Discrete role/extruder legends and gradient legends use ViewModel `legendType` and labels. |
| Renderer | `GLViewport.gcodeViewMode` and `roleVisibility` bind to ViewModel properties, never duplicated QML state. |

## Visual Rules

- The header status must remain short and must not push right-side metrics into
  overlap.
- Role and legend rows remain dense enough for the restored right analysis
  panel.
- Data-unavailable modes are honest but not alarming; the user can still view
  the toolpath using the existing fallback color.

## Non-Goals

- Do not add a new color-mode editor.
- Do not change the GCV1 payload layout.
- Do not remove the existing role visibility renderer-side mask.
