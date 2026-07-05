---
phase: 76-prepare-workflow-panels-restoration
status: approved
approved_by: autonomous
requirements: [OBJ-01, PLATEUI-01, STATUS-01]
---

# Phase 76 UI Specification

## Design Intent

The Prepare workflow panels should read as working controls around the 3D bed,
not as decorative dashboard cards. The object list, plate strip, and slice
state must be dense, stateful, and visually secondary to the viewport.

## Object List Contract

- Render object rows as compact tree rows, not large rounded cards.
- Object row target height: about 38 px.
- Volume row target height: about 26 px.
- Group headers target height: about 18 px.
- Selection uses a narrow left rail plus subtle row fill.
- Printable and disabled state must be visible on the row.
- Object actions remain gated by existing ViewModel properties.
- Context menus remain the primary surface for destructive and advanced
  object/volume actions.
- Empty state remains visible and honest when no objects are loaded.

## Plate Strip Contract

- Keep the strip bed-adjacent at the viewport bottom.
- Reduce strip height and card width so it behaves like a compact workflow
  indicator, not a row of cards.
- Active plate is indicated by accent border/fill and bold label.
- Missing, sliced, and stale result states are explicit and visually distinct.
- Add plate remains a compact icon-like button gated by `canAddPlate`.
- Right-click context and drag/drop behavior remain unchanged.

## Slice Status Contract

- Remove the floating viewport slice button. Slice/cancel must live in the
  sidebar workflow status panel and top-level shell actions.
- Primary slice/cancel action binds to backend readiness:
  - Enabled when slicing is active, or when `canRequestSlice` is true.
  - Disabled with a backend hint otherwise.
- Preview and export actions bind to `canPreview` and `canExportGCode`.
- Slice-all is not a dead button; it is disabled unless the backend can slice
  or a valid result exists.
- Progress, result, and readiness hints remain live and backed by
  `EditorViewModel`.

## Visual Acceptance

The Phase 76 pass is acceptable when the current Prepare page no longer shows
the floating viewport `>> Slice` button, the plate strip is visibly compact
and stateful, and the object list presents a dense tree/list rather than a
card stack.
