# Phase 53: Prepare Object, Plate, and Viewport Workflow - UI Spec

**Status:** Approved for planning
**Source:** Screenshot-driven v3.6 inventory plus OrcaSlicer source truth

## Scope

This UI contract covers the Prepare page object list, plate bar, viewport
overlays, view controls, and gizmo entry points. The target is a dense slicer
workspace, not a landing page or explanatory UI.

## Layout Contract

- The Prepare viewport remains the central first-viewport signal.
- The left sidebar may be collapsible/dockable, but when visible it must not
  cover the viewport or plate strip.
- The object list is a compact operational tree/list. It should support scanning
  object names, plate labels, printable state, volume children, and selection
  status without oversized cards.
- Viewport overlays must be sparse and anchored: main actions at top-left,
  gizmos as a vertical strip, view/camera controls on the right, plate strip at
  the bottom.
- The bottom plate strip must leave room for object-info banners and slice
  controls. No overlay should hide GLViewport mouse interaction across the main
  canvas.
- Floating gizmo panels must be compact tool surfaces, not nested cards.

## Interaction Contract

- Every visible command must call a real C++ viewmodel/viewport API or be
  disabled with a clear status.
- Object list row click selects; modifier/multi-selection behavior follows the
  existing `EditorViewModel` selection contract.
- Inline rename commits through `EditorViewModel::renameObject` and must not
  leave the row stuck in edit mode.
- Context menus must expose only actions valid for the current object/volume or
  selection state.
- Plate cards select the current plate on left click and open the plate menu on
  right click. Drag/drop to plate may remain only if it calls the real
  `moveSelectedObjectToPlate` path.
- View buttons call viewport view/fit APIs. Gizmo buttons set viewport gizmo
  mode only when the target tool is supported for the current selection.

## Visual Contract

- Use the existing `Theme` palette and controls. Do not introduce a one-off
  purple/blue, beige, brown/orange, or purely slate theme.
- Cards use small radii only where a repeated item or framed tool needs a
  boundary. Page sections remain unframed.
- Text inside buttons/chips must fit at desktop and narrow viewport widths.
  Prefer icon buttons with tooltips for compact toolbars.
- Preserve visible contrast for selected object rows, active plate cards, active
  gizmo buttons, stale slice markers, and disabled actions.
- Do not add decorative gradient or blob backgrounds.

## Copy Contract

- User-visible text goes through `qsTr()`.
- Source comments added or changed in this phase are English ASCII-only.
- Existing corrupted comments in touched blocks should be replaced only when the
  intended meaning is clear from the code or upstream reference.
- Disabled/blocked tooltips should be short and operational, for example
  "Requires selected object" or "Blocked: OpenVDB unavailable".

## Source Truth Anchors

- Object list/actions: `third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectList.cpp`
  and `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`.
- Plate controls: `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp`,
  `PartPlate.cpp`, and related `Plater.cpp` plate wrappers.
- View controls: `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp`.
- Gizmo entries: `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosManager.cpp`
  and per-gizmo `on_render_input_window` implementations.

## Verification Contract

- Automated QML audits must reject empty handlers, dead enabled commands, and
  unsupported gizmo entries that look active.
- Viewmodel smoke tests must cover import/add-object state propagation, object
  action gating, plate action state, and renderer-facing membership stability.
- Manual validation must include import -> select -> context-menu action ->
  plate switch -> camera rotate -> tool switch -> Preview -> Prepare return.
