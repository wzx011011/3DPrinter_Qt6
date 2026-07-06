# Phase 74: Prepare Source-Truth Gap Audit - Context

**Gathered:** 2026-07-05
**Status:** Ready for planning
**Mode:** Autonomous smart-discuss defaults

<domain>
## Phase Boundary

Phase 74 is a read-only audit phase for the v3.9 Prepare page restoration milestone.
It maps the screenshot-visible Prepare page regions to:

- the target visual screenshot,
- the current Qt runtime evidence,
- OrcaSlicer source-truth files,
- current Qt/QML targets,
- downstream phase ownership and verification method.

This phase does not modify UI source code. It produces the canonical v3.9
Prepare gap matrix that Phase 75, Phase 76, and Phase 77 must execute against.

</domain>

<decisions>
## Implementation Decisions

### Source And Visual Truth

- Use `shotScreen/准备页.png` as the visual/layout target for v3.9.
- Use `.planning/milestones/prepare-gap-current-20260704.png` as the current
  runtime baseline until Phase 78 captures fresh final evidence.
- Use OrcaSlicer source as behavior truth, especially:
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectList.cpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectSettings.cpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/PresetComboBoxes.cpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/*`

### Audit Granularity

- Keep the Phase 50 Prepare region IDs: `PREP-TOP`, `PREP-SIDEBAR`,
  `PREP-OBJLIST`, `PREP-VIEWPORT`, `PREP-VTOOLBAR`, `PREP-GIZMOFLOAT`,
  `PREP-PLATEBAR`, `PREP-SLICESTATUS`, and `PREP-VIEWOPTS`.
- For each region record target behavior, current evidence, Qt target files,
  upstream source, severity, owning v3.9 phase, requirement mapping, and
  verification.
- Treat a control as incomplete if it is visible but visually off-target,
  exposes raw/internal labels, is dead/no-op, or lacks an upstream source map.

### Downstream Ownership

- Phase 75 owns the left sidebar: printer, filament, process preset/options,
  search, scope toggles, display names, density, and placeholder removal.
- Phase 76 owns workflow panels: object list, plate strip, slice/status
  surfaces, and action availability.
- Phase 77 owns viewport-adjacent UI: main toolbar, vertical toolbars, view
  controls, gizmo floating panels, and RHI viewport presentation alignment.
- Phase 78 owns cleanup and runtime visual verification.

### the agent's Discretion

- The audit may classify visual-only issues as High when they materially block
  screenshot parity, even if the underlying behavior already works.
- The audit may defer adjacent Preview/settings/device issues unless they block
  Prepare page visual or interaction parity.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- `src/qml_gui/pages/PreparePage.qml` owns the Prepare page layout, viewport
  area, plate bar, context menus, dialogs, and overlay panels.
- `src/qml_gui/panels/DockableSidebar.qml` wraps the left sidebar and controls
  sidebar width/collapse behavior.
- `src/qml_gui/panels/LeftSidebar.qml` owns printer, filament, process options,
  object list embedding, and object settings embedding.
- `src/qml_gui/panels/ObjectList.qml` owns object/volume rows, context menus,
  selection actions, import button, and drag/reorder behavior.
- `src/qml_gui/components/GLToolbars.qml` owns main Prepare toolbar buttons,
  gizmo buttons, view preset buttons, and the floating slice button.
- `src/qml_gui/Renderer/RhiViewport*` owns the default Prepare viewport renderer.
- `src/core/viewmodels/EditorViewModel.*` and `ConfigViewModel.*` own the
  behavior and state exposed to QML.

### Established Patterns

- QML is presentation and wiring only; business rules and upstream behavior
  mapping belong in C++ viewmodels/services.
- Screenshot-driven milestones use `shotScreen/` as visual truth and upstream
  OrcaSlicer source as behavior truth.
- Replaced Prepare UI paths must not leave stale files, imports, resource
  entries, routes, tests, or disconnected controls.
- Verification for early v3.9 phases should use source/QML audits, focused
  tests, `git diff --check`, and the encoding guard. The full canonical build
  and runtime visual evidence are Phase 78 responsibilities.

### Integration Points

- `PreparePage.qml:20-23` sets the current sidebar width defaults.
- `PreparePage.qml:1593-1631` integrates `DockableSidebar` and `GLToolbars`.
- `PreparePage.qml:3009-3166` owns the current plate bar.
- `LeftSidebar.qml:52-421` owns printer, filament, and process header controls.
- `LeftSidebar.qml:428-556` owns process search and inline option rows.
- `LeftSidebar.qml:577-688` embeds object list and object settings sections.
- `ObjectList.qml:133-144`, `476-535`, and `968-1009` expose selection settings,
  export, repair, reload, and other context-menu entries.
- `GLToolbars.qml:35-344` owns the current toolbar, gizmo bar, view bar, and
  floating slice button.

</code_context>

<specifics>
## Specific Ideas

The current runtime baseline is materially different from the target screenshot:

- target left sidebar is narrow, compact, flat, and localized;
- current left sidebar is wider, card-heavy, and still exposes English/internal
  option labels such as `Layer height`, `print`, and `default`;
- target slice/export controls live in the top-right shell;
- current runtime keeps a large green floating `Slice` button in the viewport;
- target viewport uses the grey bed-centered OrcaSlicer/Creality visual style;
- current runtime uses a dark viewport, different toolbar placement, and a
  grid-only bed baseline;
- target toolbars are compact icon strips around the bed;
- current toolbars use text glyph buttons, a left vertical gizmo bar, and a
  right-edge view bar that do not match the target layout.

</specifics>

<deferred>
## Deferred Ideas

- Preview page visual closure remains outside v3.9 unless a direct Prepare
  dependency is found.
- Full parameter settings dialog restoration remains outside v3.9 except for
  Prepare sidebar entry/display needs.
- Device/cloud/Monitor, AssembleView, and backend promotion remain separate
  source-truth milestones.

</deferred>
