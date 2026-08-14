# Requirements: OWzx Slicer

**Milestone:** v5.16 Full Gap Closure
**Defined:** 2026-08-15
**Core Value:** OrcaSlicer upstream behavior is the product source of truth;
Qt6 code must inherit that behavior and must not invent new product behavior
without an explicit upstream mapping or documented block.
**Evidence base:** `.planning/research/v5.16-GAP-BASELINE.md` (2026-08-15
full-delta re-audit; every requirement below maps to a baseline gap item with
file:line evidence).

## v5.16 Requirements

### CIRC: Broken-Circuit Fixes (P0 — clickable but functionally dead)

- [ ] **CIRC-01**: User can undo/redo through every UI entry (Ctrl+Z/Y,
  topbar buttons, menus) and the last editor action actually reverts/reapplies.
- [ ] **CIRC-02**: User runs each calibration mode and the generated G-code
  sweeps the correct parameter axis (symbolic CalibMode constants; stale
  QmlUiAudit mapping assertion corrected).
- [ ] **CIRC-03**: User can execute groove/connector cuts from the AdvancedCut
  panel, place connectors interactively, and the connector toggle/click states
  stay coherent between Cut and AdvancedCut modes.
- [ ] **CIRC-04**: User can select a different filament preset per extruder
  slot, sees each slot's real selection and color, and no slot falsely reports
  incompatible (no mojibake category match).
- [ ] **CIRC-05**: User exports all objects as one STL and gets a merged mesh
  of every object, not only the first.
- [ ] **CIRC-06**: User never sees mojibake strings (export-failure title and
  any other GBK-corrupted tr() strings fixed at source).
- [ ] **CIRC-07**: CLI `--plate` behavior matches its help (0 = all plates).

### PLATE: Plate Data-Chain Fixes (P0)

- [ ] **PLATE-01**: User changes a plate's bed type and the sliced output
  reflects it (config sync into slicing, upstream `curr_bed_type` semantics).
- [ ] **PLATE-02**: User changes a plate's spiral mode and the sliced output
  reflects it.
- [ ] **PLATE-03**: User edits per-plate first/other-layer filament sequence
  and it reaches slicing, persists to 3MF, and round-trips on reopen.
- [ ] **PLATE-04**: User opens a multi-plate 3MF and every object returns to
  its saved plate (no forced 220x220 re-arrange; upstream exact restore).
- [ ] **PLATE-05**: User sees an accurate unsaved-changes indicator driven by
  real edits and gets a guard dialog on new/open/close with unsaved changes.

### UNDO: Undo Coverage And Fidelity (P0/P1)

- [ ] **UNDO-01**: User deletes an object and undo restores the full mesh,
  transforms, and volume structure (redo by object identity, not name).
- [ ] **UNDO-02**: User deletes a volume and undo restores it.
- [ ] **UNDO-03**: User adds/deletes/moves/clones/locks a plate and undo
  restores the previous plate state.
- [ ] **UNDO-04**: User paints support/seam/MMU facets and undo reverts the
  paint strokes.
- [ ] **UNDO-05**: User pastes a copied object and gets mesh, transforms, and
  config overrides — not a name-only empty object; show/hide no longer writes
  printability.
- [ ] **UNDO-06**: User edits object-level layer ranges and the ranges reach
  `layer_config_ranges` so variable layer height affects slicing.

### PSET2: Preset System Completion (P0/P1)

- [ ] **PSET2-01**: User creates/edits a preset and it persists to disk in
  the upstream user-directory JSON format, surviving restart.
- [ ] **PSET2-02**: User creates a preset of the chosen category with the
  chosen inheritance in CreatePresetsDialog (scope mapping + inherits honored).
- [ ] **PSET2-03**: User triggers a dirty guard and sees exactly one modal
  with Save / Transfer / Discard / Cancel and per-item selection (upstream
  UnsavedChangesDialog semantics).
- [ ] **PSET2-04**: User imports/exports preset bundles in a form upstream
  OrcaSlicer can read, with correct category mapping.
- [ ] **PSET2-05**: User sees preset combos sectioned (Project/User/System)
  with incompatible presets grayed in-list (upstream PresetComboBoxes).
- [ ] **PSET2-06**: User configures per-extruder filament presets backed by a
  real multi-slot data model (upstream `filament_presets` vector semantics).
- [ ] **PSET2-07**: User renames/deletes presets from the UI with upstream
  guards ("in use" protection).
- [ ] **PSET2-08**: User sees no permanently-empty settings tabs (Printer
  Material/Extruder, Filament Overrides/Multimaterial/Dependencies either
  populated or removed).

### DLG: Dialog Reachability And Completion (P1)

- [ ] **DLG-01**: User can open every implemented dialog from the UI
  (BedShape, EditGCode, AMS, Firmware, SpeedLimit, WipeTower, PluginManager,
  EnableLiteMode, CreatePresets, ExportBundle) or it is honestly gated.
- [ ] **DLG-02**: User saves custom G-code from EditGCodeDialog and it lands
  in the config; WipeTower flush matrix OK persists to the preset.
- [ ] **DLG-03**: User gets the upstream load-feedback dialogs: archive file
  tree selection (FileArchive), out-of-bed recenter prompt (RecenterDialog),
  OBJ color mapping (ObjColorDialog), 3MF version/incompatible/legacy
  warnings, SysInfo viewer, single-choice dialog.
- [ ] **DLG-04**: User sees the correct license and copyright in AboutDialog
  (upstream AGPL).

### VIEW: Menus, Shortcuts, Drag-Drop, Import (P1)

- [ ] **VIEW-01**: User can use the View menu: 6 camera presets (Ctrl+0..6),
  perspective/orthographic toggle, G-code window toggle, overhang and outline
  display.
- [ ] **VIEW-02**: User can Delete All (Ctrl+D), import zip archives and
  config files, and the 11 missing upstream shortcuts are bound
  (Ctrl+N/Shift+S/A/Esc/P/E/C…).
- [ ] **VIEW-03**: User can drag model files onto the window to import them
  (multi-file, SVG point-drop included).
- [ ] **VIEW-04**: User importing a model saved in wrong units gets the
  upstream meters/inches detection and conversion prompt; zero-volume objects
  are reported.
- [ ] **VIEW-05**: User opens a project 3MF and its embedded config and
  presets are restored (LoadModel|LoadConfig semantics).
- [ ] **VIEW-06**: User can export the plate sliced file (.gcode.3mf) and
  scale selection to fit the build volume.

### PREV: Preview Completion (P1)

- [ ] **PREV-01**: User sees ghost object shells behind toolpaths in preview
  (upstream render_shells).
- [ ] **PREV-02**: User sees the 3D tool-position marker rendered at the
  current move position.
- [ ] **PREV-03**: User can show/hide Seam, Retract, Unretract, and Wipe
  moves, parsed and rendered like upstream.
- [ ] **PREV-04**: User can add a filament change (ToolChange) from the layer
  rail with extruder/color picker, sees tick hover tooltips, and gets the
  Jump-to-Layer dialog.
- [ ] **PREV-05**: User sees filament statistics split (Model/Support/
  Flushed/Tower), a real stealth-mode estimate, and configurable cost.
- [ ] **PREV-06**: User sees Tool/ColorPrint modes using the configured
  extruder colors with per-extruder visibility toggles.
- [ ] **PREV-07**: User on the software-render fallback still sees the
  G-code preview.

### ENGN: Slicing Engine Semantics (P1)

- [ ] **ENGN-01**: User switches to Preview with stale results and the app
  auto-reslices (upstream do_reslice).
- [ ] **ENGN-02**: User re-enters preview with valid results and the app
  reuses the existing G-code without re-slicing (loadGCodeFromPrevious path).
- [ ] **ENGN-03**: User exports G-code without the UI freezing, and slicing
  validation warnings (not just errors) are surfaced.
- [ ] **ENGN-04**: User running slice-all on a project with an empty plate
  does not lose the remaining plates' results.

### NOTI: Notification System (P2)

- [ ] **NOTI-01**: User sees multiple notifications stacked with upstream
  priority ordering, duplicate compression with counters, and hyperlink
  actions where upstream provides them.

### GIZ: Gizmo Interaction Depth (P2)

- [ ] **GIZ-01**: User can type exact position/rotation/scale values in the
  gizmo mini panel (world/local coordinates).
- [ ] **GIZ-02**: User can smart-fill/seed-fill painter gizmo selections and
  filter by overhangs/angle (upstream TriangleSelector smart fill).
- [ ] **GIZ-03**: User sees hovered face highlight on Flatten and can click a
  specific face to place the object on.
- [ ] **GIZ-04**: User can drag and rotate the cutting plane directly in 3D.
- [ ] **GIZ-05**: User sees measure annotations rendered in the 3D scene.
- [ ] **GIZ-06**: User can edit an existing text volume in place, preview
  simplification before applying (with cancel), and use the SVG drop workbench.

### PAGE: Top-Level Page Honesty (P2)

- [ ] **PAGE-01**: User's home page shows real recent projects (clickable)
  and working quick actions; DailyTips rotate with upstream hint data.
- [ ] **PAGE-02**: User sees a Project page without dead controls (no
  console.log buttons, real file tree/details, or honest gating).
- [ ] **PAGE-03**: User can run PA_Pattern/PA_Tower calibration modes, the
  two-pass FlowRate flow, save calibration results into presets, and keep
  calibration history across restarts.
- [ ] **PAGE-04**: User's preferences actually take effect (default start
  page, units, backup) or are honestly gated with a reason.

### CLI: Command-Line Surface (P2)

- [ ] **CLI-01**: User can run upstream CLI transforms (arrange, orient, cut,
  split, assemble, repair, scale-to-fit) and exports (STL/3MF/slicedata).
- [ ] **CLI-02**: User can override any PrintConfig key from the CLI.

## Carried: v5.11 Process Settings (reserved, deferred)

The v5.11 requirements (HIER-03, DISC-01..04, FILT-01..03, ROW-01..05,
PSET-01..02, VER-01) remain valid and mapped to reserved Phases 227-230.
They are preserved in git history (REQUIREMENTS.md @ v5.11 tag) and are NOT
part of v5.16 phase mapping. v5.16 phases must not use numbers 227-230.

## Out of Scope

| Exclusion | Reason |
|-----------|--------|
| Device, hardware, network, cloud, camera, monitor, mall WebView, multi-machine send | Removed product scope (2026-07-07) unless explicitly reopened. |
| SLA settings or slicing | Permanently removed (2026-07-26). |
| libslic3r algorithm changes | GUI migration only; engine consumed as-is. |
| Object group/ungroup | Upstream OrcaSlicer has no such action — not a gap. |
| GLGizmoLayerHeight editor | Not present in this upstream snapshot — not a gap. |
| Unmapped cross-option auto-correction | Requires its own source mapping and behavior tests. |
| v5.11 Process dialog work (Phases 227-230) | Reserved workstream, resumed separately. |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| CIRC-01 | 231. P0 Circuit Break Quick Fixes | Planned |
| CIRC-02 | 231. P0 Circuit Break Quick Fixes | Planned |
| CIRC-05 | 231. P0 Circuit Break Quick Fixes | Planned |
| CIRC-06 | 231. P0 Circuit Break Quick Fixes | Planned |
| CIRC-07 | 231. P0 Circuit Break Quick Fixes | Planned |
| CIRC-03 | 232. P0 AdvancedCut Execution And Multi-Slot Filaments | Planned |
| CIRC-04 | 232. P0 AdvancedCut Execution And Multi-Slot Filaments | Planned |
| PLATE-01 | 233. P0 Plate Data Chain | Planned |
| PLATE-02 | 233. P0 Plate Data Chain | Planned |
| PLATE-03 | 233. P0 Plate Data Chain | Planned |
| PLATE-04 | 233. P0 Plate Data Chain | Planned |
| PLATE-05 | 233. P0 Plate Data Chain | Planned |
| ENGN-04 | 233. P0 Plate Data Chain | Planned |
| UNDO-01 | 234. Undo Coverage And Fidelity | Planned |
| UNDO-02 | 234. Undo Coverage And Fidelity | Planned |
| UNDO-03 | 234. Undo Coverage And Fidelity | Planned |
| UNDO-04 | 234. Undo Coverage And Fidelity | Planned |
| UNDO-05 | 234. Undo Coverage And Fidelity | Planned |
| UNDO-06 | 234. Undo Coverage And Fidelity | Planned |
| PSET2-01 | 235. Preset System Completion | Planned |
| PSET2-02 | 235. Preset System Completion | Planned |
| PSET2-03 | 235. Preset System Completion | Planned |
| PSET2-04 | 235. Preset System Completion | Planned |
| PSET2-05 | 235. Preset System Completion | Planned |
| PSET2-06 | 235. Preset System Completion | Planned |
| PSET2-07 | 235. Preset System Completion | Planned |
| PSET2-08 | 235. Preset System Completion | Planned |
| DLG-01 | 236. Dialog Reachability And Completion | Planned |
| DLG-02 | 236. Dialog Reachability And Completion | Planned |
| DLG-03 | 236. Dialog Reachability And Completion | Planned |
| DLG-04 | 236. Dialog Reachability And Completion | Planned |
| VIEW-01 | 237. Menus, Shortcuts, Drag-Drop, And Import | Planned |
| VIEW-02 | 237. Menus, Shortcuts, Drag-Drop, And Import | Planned |
| VIEW-03 | 237. Menus, Shortcuts, Drag-Drop, And Import | Planned |
| VIEW-04 | 237. Menus, Shortcuts, Drag-Drop, And Import | Planned |
| VIEW-05 | 237. Menus, Shortcuts, Drag-Drop, And Import | Planned |
| VIEW-06 | 237. Menus, Shortcuts, Drag-Drop, And Import | Planned |
| PREV-01 | 238. Preview Completion | Planned |
| PREV-02 | 238. Preview Completion | Planned |
| PREV-03 | 238. Preview Completion | Planned |
| PREV-04 | 238. Preview Completion | Planned |
| PREV-05 | 238. Preview Completion | Planned |
| PREV-06 | 238. Preview Completion | Planned |
| PREV-07 | 238. Preview Completion | Planned |
| ENGN-01 | 239. Slicing Engine Semantics | Planned |
| ENGN-02 | 239. Slicing Engine Semantics | Planned |
| ENGN-03 | 239. Slicing Engine Semantics | Planned |
| NOTI-01 | 240. Notification Stacking And Gizmo Interaction Depth | Planned |
| GIZ-01 | 240. Notification Stacking And Gizmo Interaction Depth | Planned |
| GIZ-02 | 240. Notification Stacking And Gizmo Interaction Depth | Planned |
| GIZ-03 | 240. Notification Stacking And Gizmo Interaction Depth | Planned |
| GIZ-04 | 240. Notification Stacking And Gizmo Interaction Depth | Planned |
| GIZ-05 | 240. Notification Stacking And Gizmo Interaction Depth | Planned |
| GIZ-06 | 240. Notification Stacking And Gizmo Interaction Depth | Planned |
| PAGE-01 | 241. Page Honesty And CLI Surface | Planned |
| PAGE-02 | 241. Page Honesty And CLI Surface | Planned |
| PAGE-03 | 241. Page Honesty And CLI Surface | Planned |
| PAGE-04 | 241. Page Honesty And CLI Surface | Planned |
| CLI-01 | 241. Page Honesty And CLI Surface | Planned |
| CLI-02 | 241. Page Honesty And CLI Surface | Planned |

**Coverage:**
- v5.16 requirements: 60 total (actual REQ-ID count; corrects the earlier 55 figure)
- Mapped to phases: 60 (100%)
- Unmapped: 0

---
*Requirements defined: 2026-08-15*
*Last updated: 2026-08-15 after v5.16 milestone start*
