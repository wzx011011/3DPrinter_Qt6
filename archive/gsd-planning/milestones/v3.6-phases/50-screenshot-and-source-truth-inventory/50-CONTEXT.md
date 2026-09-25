# Phase 50: Screenshot and Source-Truth Inventory - Context

**Gathered:** 2026-07-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 50 produces the visual/behavior contract that gates all downstream v3.6
implementation phases. It maps every screenshot-visible region in
`shotScreen/准备页.png`, `shotScreen/预览页.png`,
`shotScreen/打印机参数设置页.png`, and `shotScreen/材料参数设置页.png` to a Qt
target file/component, an upstream OrcaSlicer source file (and symbol where
obvious), a behavior status, a verification method, and a modify-vs-replace
decision with cleanup checklist.

Phase 50 is documentation/contract only. No QML, C++, route, resource, type
registration, or test file is modified in this phase. Implementation work
begins in Phase 51 (Shell and Navigation Restoration).

</domain>

<decisions>
## Implementation Decisions

### Inventory Document Format
- One canonical file at `docs/v3.6-ui-inventory.md`, plus per-phase excerpt
  tables copied into each downstream phase's CONTEXT as that phase is planned.
  The canonical doc is the single source of truth; phase excerpts are
  convenience copies.
- Each screenshot is decomposed into named regions (top-shell, left-sidebar,
  viewport, right-panel, bottom-slider, dialog-tabs, etc.) using a fixed
  column schema. Region granularity targets user-visible modules, not
  individual controls (a control-level map would be unmanageably large).
- Column schema per row: `region_id, region_name, visible_controls,
  qt_target, upstream_source, status, verification, modify_or_replace,
  cleanup`.
- One modify-vs-replace decision per region/module, with a cleanup checklist
  per replacement decision (old files, routes, resources, registrations,
  imports, tests to remove).

### Status Vocabulary and Verification Methods
- Status vocabulary uses the 6 REQUIREMENTS terms (Real / Hybrid / Mock /
  Blocked / Placeholder / Superseded) plus a `Missing` term for Qt targets
  that do not yet exist. No numeric maturity score.
- Verification method per row is one of: `automated-test`,
  `deterministic-harness`, `manual-visual`, `manual-uat-checklist`,
  `build-only`, `upstream-parity-audit`. Free text is not used.
- Upstream source refs go to file level plus symbol/region where obvious
  (e.g., `Plater.cpp Sidebar::*`). Line numbers are NOT used because they
  drift against the v7.0.1 lock as upstream moves.
- Screenshot regions use stable IDs prefixed by page:
  `PREP-TOP`, `PREP-SIDEBAR`, `PREP-VIEWPORT`, `PREP-OBJLIST`,
  `PREP-VTOOLBAR`, `PREV-LEFT`, `PREV-VIEWPORT`, `PREV-VSLIDER`,
  `PREV-MSLIDER`, `PREV-RIGHT`, `SET-PRINTER-TABS`, `SET-MATERIAL-TABS`,
  etc. Region display names may be bilingual (English primary, Chinese in
  parentheses where useful) but IDs are ASCII-only.

### Scope Boundaries and Deliverable Form
- Phase 50 DOES visually analyze each screenshot (read pixels/layout) to
  extract regions, then maps each region. Screenshots are the visual truth
  per PROJECT.md.
- Deliverable artifacts: canonical `docs/v3.6-ui-inventory.md` (long-lived
  reference) plus `.planning/phases/50-screenshot-and-source-truth-inventory/
  50-INVENTORY.md` (the verified phase contract with a sign-off summary,
  traceability to INV-01..INV-05, and cleanup-checklist aggregates).
- Phase 50 DOES produce the modify-vs-replace decision per region (INV-05).
  Each replacement decision carries a cleanup checklist. Execution of those
  decisions/checklists happens in Phases 51-57, not here.
- Out of scope: any code change (QML, C++, CMake), any qml.qrc / route /
  type-registration / test change, any dependency update. Pure
  documentation/contract work only.

### Claude's Discretion
- Exact region count and boundaries per screenshot (within reason; 6-12
  regions per screenshot is the expected density).
- Choice of internal section ordering inside the canonical doc.
- Whether to include small reference crop coordinates for a region (e.g.,
  approximate pixel rect) as an optional helper column when the region is
  visually ambiguous.
- Bilingual vs English-only display names per region.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing docs reference matrix: `docs/源码真值功能矩阵.md` and
  `docs/源码真值基线.md` contain prior upstream-to-Qt mapping notes that
  can seed the upstream_source column.
- `docs/顶部工具栏验收清单.md` already covers the top toolbar acceptance
  items and can seed PREP-TOP / PREV-TOP region rows.
- `docs/项目结构.md` documents the current Qt project structure (pages,
  panels, components, dialogs) and seeds the qt_target column.

### Established Patterns
- Qt side is organized as: `src/qml_gui/pages/` (top-level pages incl.
  PreparePage.qml, PreviewPage.qml, ConfigPage.qml, SettingsPage.qml),
  `src/qml_gui/panels/` (LeftSidebar, Sidebar, ObjectList, PrintSettings,
  FilamentPanel, SliceProgress), `src/qml_gui/components/` (Legend,
  StatsPanel, LayerSlider, MoveSlider, Toolbar, GLToolbars, etc.),
  `src/qml_gui/dialogs/` (UnsavedChangesDialog, SavePresetDialog,
  BedShapeDialog, etc.).
- Viewmodels live in `src/core/viewmodels/` (EditorViewModel,
  PreviewViewModel, ConfigViewModel, etc.). Services in
  `src/core/services/` (ProjectServiceMock, PresetServiceMock, SliceService).
- Upstream source-truth candidates already enumerated in PROJECT.md Context
  section: Prepare -> `Plater.*`, `GLCanvas3D.*`, `GUI_ObjectList.*`,
  `GUI_ObjectSettings.*`, `Gizmos/*`; Preview -> `GUI_Preview.*`,
  `GCodeViewer.*`, `GLCanvas3D.*`, `libslic3r/GCode/*`; Settings ->
  `Tab.*`, `PresetComboBoxes.*`, `ConfigManipulation.*`,
  `UnsavedChangesDialog.*`, `CreatePresetsDialog.*`, `PrintConfig.*`,
  `Preset.*`, `PresetBundle.*`. These are confirmed to exist on disk
  (Plater.cpp/hpp, GCodeViewer.cpp/hpp verified; Gizmos/ has 20+ gizmo
  files).

### Integration Points
- Canonical doc `docs/v3.6-ui-inventory.md` will be referenced by every
  downstream phase (51-57) during their discuss/plan steps, and by Phase 58
  for traceability (VERIFY-01 covers inventory completeness).
- Phase 50's `50-INVENTORY.md` becomes the phase contract that Phase 50
  verification (deterministic harness checking doc presence + schema) gates on.

</code_context>

<specifics>
## Specific Ideas

- Region IDs must be ASCII-only and stable so downstream phases and tests can
  reference them without unicode/escaping issues. Chinese is allowed only in
  display names and prose.
- The cleanup checklist per replacement decision must be machine-grep-friendly
  (one item per line, file paths verbatim) so Phase 57 (Deprecated UI Removal)
  and Phase 58 tests can consume it directly.
- The `Missing` status term is required because several screenshot regions
  (e.g., restored Settings dialogs as independent windows) do not yet have a
  Qt target at all; mapping them honestly is more useful than forcing a
  nearest-existing-file match.

</specifics>

<deferred>
## Deferred Ideas

- Optional JSON/YAML sidecar mirror of the inventory for programmatic test
  consumption — deferred; markdown tables with stable IDs are sufficient for
  Phase 58 tests, and a sidecar would need its own sync discipline.
- Precise upstream line-number references — deferred because line numbers
  drift against the v7.0.1 lock.
- Screenshot pixel-rect crop images per region — deferred as a Phase 51+
  concern if a downstream phase needs them; optional helper-column coordinates
  are allowed at Claude's discretion but not required.

</deferred>
