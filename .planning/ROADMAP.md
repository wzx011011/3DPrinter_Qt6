# Roadmap: OWzx Slicer

## Overview

v5.16 Full Gap Closure closes every gap recorded by the 2026-08-15 full-delta
re-audit (`.planning/research/v5.16-GAP-BASELINE.md`): the P0 fake-green
circuit breaks (undo/redo dispatch, CalibMode enum shift, AdvancedCut wiring,
multi-filament slots, mojibake, export-all-STL), the P0 data-link breaks
(plate bed-type/spiral/filament-sequence config chain, 3MF load membership,
delete-undo mesh loss, preset persistence, triple dirty-guard), the P1
feature fill (dialog reachability, View menu, drag&drop, import pipeline,
preview gaps, undo coverage, engine reslice semantics), and the P2 interaction
depth (notification stacking, gizmo numeric/smart-fill/measure, top-level
page honesty, CLI transforms). Phases 231-241; every v5.16 requirement maps
to exactly one phase.

## Milestone History

- Historical milestones v2.9 through v5.15 are complete; see
  `.planning/MILESTONES.md` for their archived history.
- Complete: v5.11 Phase 226 (2026-08-11); v5.14 UI Visual Parity (2026-08-14);
  v5.15 Bed Texture & Model Lighting (2026-08-14).
- **Reserved/deferred:** v5.11 Process Settings Phases 227-230 remain reserved
  for the deferred v5.11 workstream (Phase 227 planning artifact exists at
  `.planning/phases/227-process-projection-disclosure-and-filtering/`).
  These numbers must NOT be renumbered, reused, or consumed by v5.16;
  v5.16 phases therefore start at 231.

## Phases

- [x] **Phase 231: P0 Circuit Break Quick Fixes** - Make the visibly dead
  quick-fix circuits real: undo/redo dispatch, CalibMode correctness,
  export-all-STL merge, mojibake, CLI `--plate 0`.
- [x] **Phase 232: P0 AdvancedCut Execution And Multi-Slot Filaments** - Wire
  AdvancedCut execution/connector interaction and real per-extruder slot
  filament selection.
- [x] **Phase 233: P0 Plate Data Chain** - Sync plate bed type, spiral, and
  filament sequence into slicing/3MF; exact 3MF plate restore; unsaved-changes
  guard; slice-all empty-plate safety.
- [x] **Phase 234: Undo Coverage And Fidelity** - Undo restores meshes,
  volumes, plates, paint strokes, and full paste payloads; layer ranges reach
  slicing.
- [x] **Phase 235: Preset System Completion** - Disk persistence, honest
  CreatePresets scope, single dirty-guard modal, bundle interop, sectioned
  combos, multi-slot model, rename/delete guards, no empty tabs.
- [x] **Phase 236: Dialog Reachability And Completion** - Every implemented
  dialog reachable or honestly gated; edits land in config; upstream
  load-feedback dialogs; correct AGPL About.
- [x] **Phase 237: Menus, Shortcuts, Drag-Drop, And Import** - View menu,
  Delete All, missing shortcuts, window drag&drop, unit inference, embedded
  config restore, .gcode.3mf export, scale-to-fit.
- [ ] **Phase 238: Preview Completion** - Ghost shells, tool marker,
  Seam/Retract/Unretract/Wipe rendering, ToolChange tick UX, honest
  statistics, configured extruder colors, software fallback preview.
- [ ] **Phase 239: Slicing Engine Semantics** - Auto-reslice on stale preview
  switch, G-code reuse on re-entry, non-blocking export with surfaced
  validation warnings.
- [ ] **Phase 240: Notification Stacking And Gizmo Interaction Depth** -
  Stacked priority notifications; gizmo numeric input, smart fill, Flatten
  face pick, 3D cut-plane drag, measure overlay, Emboss/Simplify/SVG depth.
- [ ] **Phase 241: Page Honesty And CLI Surface** - Real Home/Project/
  Calibration/Preferences behavior; upstream CLI transforms, exports, and
  PrintConfig overrides.

## Phase Details

### Phase 231: P0 Circuit Break Quick Fixes
**Goal**: Users stop hitting clickable-but-dead controls for the quick,
surgical P0 breaks; what the UI claims now happens.
**Depends on**: Nothing (first v5.16 phase)
**Requirements**: CIRC-01, CIRC-02, CIRC-05, CIRC-06, CIRC-07
**Success Criteria** (what must be TRUE):
  1. User presses Ctrl+Z / Ctrl+Y, the topbar buttons, or the menu entries and
     the last editor action actually reverts/reapplies through the real
     `EditorViewModel` undo/redo — the repaint-only no-op path is gone.
  2. User runs each calibration mode and the generated G-code sweeps the
     parameter axis the mode names (FlowRate sweeps flow, TempTower sweeps
     temperature, VolSpeed/VFA/Retraction each sweep their own axis),
     dispatched via symbolic CalibMode constants, and the stale QmlUiAudit
     mapping assertion is corrected to the symbolic mapping.
  3. User exports "all objects as one STL" and the output file contains a
     merged mesh of every object, not only the first.
  4. User never sees mojibake strings: the export-failure title and every
     other GBK-corrupted tr() source string are fixed at source.
  5. CLI `--plate 0` slices all plates, matching its documented help text.
**Plans**: 2 plans
**UI hint**: yes

### Phase 232: P0 AdvancedCut Execution And Multi-Slot Filaments
**Goal**: Users can actually execute AdvancedCut operations and trust the
per-extruder filament slot UI.
**Depends on**: Phase 231
**Requirements**: CIRC-03, CIRC-04
**Success Criteria** (what must be TRUE):
  1. User executes groove/connector cuts from the AdvancedCut panel
     (`advCutSelected` reached from QML) and the resulting mesh matches the
     panel settings.
  2. User places connectors interactively and the connector toggle/click
     states stay coherent when switching between Cut and AdvancedCut modes.
  3. User selects a different filament preset per extruder slot and each slot
     shows its own real selection and color (no hardcoded slot colors, no
     shared single preset request).
  4. No slot falsely reports incompatible: the mojibake category match
     (`tr("鑰楁潗")`) is replaced by the correct mapping so the耗材 category
     lookup succeeds.
**Plans**: 2 plans
**UI hint**: yes

### Phase 233: P0 Plate Data Chain
**Goal**: Plate-level settings and 3MF membership flow all the way to
slicing, persistence, and reload without silent loss.
**Depends on**: Phase 231
**Requirements**: PLATE-01, PLATE-02, PLATE-03, PLATE-04, PLATE-05, ENGN-04
**Success Criteria** (what must be TRUE):
  1. User changes a plate's bed type and the sliced output reflects it
     (upstream `curr_bed_type` semantics synced into the slicing config).
  2. User changes a plate's spiral mode and the sliced output reflects it.
  3. User edits per-plate first/other-layer filament sequence and it reaches
     slicing, persists to 3MF, and round-trips on reopen.
  4. User opens a multi-plate 3MF and every object returns to its saved plate
     (no forced 220x220 re-arrange; upstream exact restore of membership and
     tail plates).
  5. User sees an accurate unsaved-changes indicator driven by real edits and
     gets a guard dialog on new/open/close with unsaved changes; slice-all on
     a project containing an empty plate does not lose the remaining plates'
     results.
**Plans**: 2 plans
**UI hint**: yes

### Phase 234: Undo Coverage And Fidelity
**Goal**: Undo restores real data — meshes, volume structure, plate state,
paint strokes, and full paste payloads — instead of empty shells.
**Depends on**: Phase 233
**Requirements**: UNDO-01, UNDO-02, UNDO-03, UNDO-04, UNDO-05, UNDO-06
**Success Criteria** (what must be TRUE):
  1. User deletes an object and undo restores the full mesh, transforms, and
     volume structure; redo deletes by object identity, so renamed objects are
     never hit by mistake.
  2. User deletes a volume and undo restores it.
  3. User adds/deletes/moves/clones/locks a plate and undo restores the
     previous plate state.
  4. User paints support/seam/MMU facets and undo reverts the paint strokes.
  5. User pastes a copied object and gets mesh, transforms, and config
     overrides (not a name-only empty object) with show/hide no longer writing
     printability; user edits object-level layer ranges and the ranges reach
     `layer_config_ranges` so variable layer height affects slicing.
**Plans**: 2 plans
**UI hint**: yes

### Phase 235: Preset System Completion
**Goal**: The preset system behaves like upstream PresetBundle: persistent,
honest in creation and guarding, interoperable, and completely surfaced.
**Depends on**: Phase 232, Phase 233
**Requirements**: PSET2-01, PSET2-02, PSET2-03, PSET2-04, PSET2-05, PSET2-06, PSET2-07, PSET2-08
**Success Criteria** (what must be TRUE):
  1. User creates/edits a preset and it persists to disk in the upstream
     user-directory JSON format and survives restart.
  2. User creates a preset of the chosen category with the chosen inheritance
     in CreatePresetsDialog (scope→category mapping fixed, inherits honored),
     and a dirty switch opens exactly one modal with Save / Transfer / Discard
     / Cancel and per-item selection (upstream UnsavedChangesDialog semantics).
  3. User imports/exports preset bundles in a form upstream OrcaSlicer can
     read, with correct category mapping.
  4. User sees preset combos sectioned (Project/User/System) with incompatible
     presets grayed in-list, and can rename/delete presets from the UI with
     upstream "in use" protection.
  5. User configures per-extruder filament presets backed by a real
     multi-slot data model (upstream `filament_presets` vector semantics), and
     no settings tab stays permanently empty (Printer Material/Extruder and
     Filament Overrides/Multimaterial/Dependencies are populated or removed).
**Plans**: 3 plans
**UI hint**: yes

### Phase 236: Dialog Reachability And Completion
**Goal**: Every implemented dialog is reachable and does something real, and
the upstream load-feedback dialogs exist.
**Depends on**: Phase 235
**Requirements**: DLG-01, DLG-02, DLG-03, DLG-04
**Success Criteria** (what must be TRUE):
  1. User can open every implemented dialog from the UI (BedShape,
     EditGCode, AMS, Firmware, SpeedLimit, WipeTower, PluginManager,
     EnableLiteMode, CreatePresets, ExportBundle) or it is honestly gated
     with a visible reason.
  2. User saves custom G-code from EditGCodeDialog and it lands in the
     config; the WipeTower flush-matrix OK persists to the preset.
  3. User gets the upstream load-feedback dialogs: archive file-tree
     selection (FileArchive), out-of-bed recenter prompt (RecenterDialog),
     OBJ color mapping (ObjColorDialog), 3MF version/incompatible/legacy
     warnings, SysInfo viewer, and the single-choice dialog.
  4. User sees the correct upstream AGPL license and copyright in
     AboutDialog.
**Plans**: 2 plans
**UI hint**: yes

### Phase 237: Menus, Shortcuts, Drag-Drop, And Import
**Goal**: The main-frame interaction surface matches upstream: View menu,
full shortcuts, window drag&drop, and a real import pipeline.
**Depends on**: Phase 236
**Requirements**: VIEW-01, VIEW-02, VIEW-03, VIEW-04, VIEW-05, VIEW-06
**Success Criteria** (what must be TRUE):
  1. User can use the View menu: 6 camera presets (Ctrl+0..6),
     perspective/orthographic toggle, G-code window toggle, overhang and
     outline display.
  2. User can Delete All (Ctrl+D), import zip archives and config files, and
     the 11 missing upstream shortcuts are bound (Ctrl+N/Shift+S/A/Esc/P/E/
     C…).
  3. User can drag model files onto the window to import them (multi-file,
     SVG point-drop included).
  4. User importing a model saved in wrong units gets the upstream
     meters/inches detection and conversion prompt, zero-volume objects are
     reported, and opening a project 3MF restores its embedded config and
     presets (LoadModel|LoadConfig semantics).
  5. User can export the plate sliced file (.gcode.3mf) and scale the
     selection to fit the build volume.
**Plans**: 2 plans
**UI hint**: yes

### Phase 238: Preview Completion
**Goal**: Preview shows everything upstream shows for the same G-code and
offers the full tick/statistics workflow.
**Depends on**: Phase 235
**Requirements**: PREV-01, PREV-02, PREV-03, PREV-04, PREV-05, PREV-06, PREV-07
**Success Criteria** (what must be TRUE):
  1. User sees ghost object shells behind toolpaths in preview (upstream
     render_shells).
  2. User sees the 3D tool-position marker rendered at the current move
     position.
  3. User can show/hide Seam, Retract, Unretract, and Wipe moves, parsed and
     rendered like upstream.
  4. User can add a filament change (ToolChange) from the layer rail with
     extruder/color picker, sees tick hover tooltips, and gets the
     Jump-to-Layer dialog.
  5. User sees filament statistics split (Model/Support/Flushed/Tower), a
     real stealth-mode estimate, and configurable cost; Tool/ColorPrint modes
     use the configured extruder colors with per-extruder visibility toggles;
     the software-render fallback still shows the G-code preview.
**Plans**: 3 plans
**UI hint**: yes

### Phase 239: Slicing Engine Semantics
**Goal**: Preview/slice lifecycle decisions match upstream: reslice when
stale, reuse when valid, and never block or hide warnings on export.
**Depends on**: Phase 233
**Requirements**: ENGN-01, ENGN-02, ENGN-03
**Success Criteria** (what must be TRUE):
  1. User switches to Preview with stale results and the app auto-reslices
     (upstream do_reslice behavior).
  2. User re-enters preview with valid results and the app reuses the
     existing G-code without re-slicing (loadGCodeFromPrevious path).
  3. User exports G-code without the UI freezing (export off the GUI thread)
     and slicing validation warnings — not just errors — are surfaced.
**Plans**: 2 plans
**UI hint**: yes

### Phase 240: Notification Stacking And Gizmo Interaction Depth
**Goal**: Notifications and gizmos reach upstream interaction depth: stacked
notifications and the missing gizmo interaction layers.
**Depends on**: Phase 238
**Requirements**: NOTI-01, GIZ-01, GIZ-02, GIZ-03, GIZ-04, GIZ-05, GIZ-06
**Success Criteria** (what must be TRUE):
  1. User sees multiple notifications stacked with upstream priority
     ordering, duplicate compression with counters, and hyperlink actions
     where upstream provides them.
  2. User can type exact position/rotation/scale values in the gizmo mini
     panel (world/local coordinates), drag and rotate the cutting plane
     directly in 3D, and see hovered-face highlight on Flatten with
     click-to-place face selection.
  3. User can smart-fill/seed-fill painter gizmo selections filtered by
     overhangs/angle (upstream TriangleSelector smart fill) and sees measure
     annotations rendered in the 3D scene.
  4. User can edit an existing text volume in place, preview simplification
     before applying (with cancel), and use the SVG drop workbench.
**Plans**: 3 plans
**UI hint**: yes

### Phase 241: Page Honesty And CLI Surface
**Goal**: Top-level pages do what they show and the CLI covers the upstream
transform/export surface.
**Depends on**: Phase 235, Phase 240
**Requirements**: PAGE-01, PAGE-02, PAGE-03, PAGE-04, CLI-01, CLI-02
**Success Criteria** (what must be TRUE):
  1. User's home page shows real recent projects (clickable) and working
     quick actions; DailyTips rotate with upstream hint data.
  2. User sees a Project page without dead controls (no console.log buttons;
     real file tree/details or honest gating).
  3. User can run PA_Pattern/PA_Tower calibration modes, the two-pass
     FlowRate flow, save calibration results into presets, and keep
     calibration history across restarts.
  4. User's preferences actually take effect (default start page, units,
     backup) or are honestly gated with a reason.
  5. User can run upstream CLI transforms (arrange, orient, cut, split,
     assemble, repair, scale-to-fit) and exports (STL/3MF/slicedata), and
     override any PrintConfig key from the CLI.
**Plans**: 2 plans
**UI hint**: yes

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 231. P0 Circuit Break Quick Fixes | 0/2 | Not started | - |
| 232. P0 AdvancedCut Execution And Multi-Slot Filaments | 0/2 | Not started | - |
| 233. P0 Plate Data Chain | 0/2 | Not started | - |
| 234. Undo Coverage And Fidelity | 0/2 | Not started | - |
| 235. Preset System Completion | 0/3 | Not started | - |
| 236. Dialog Reachability And Completion | 0/2 | Not started | - |
| 237. Menus, Shortcuts, Drag-Drop, And Import | 0/2 | Not started | - |
| 238. Preview Completion | 0/3 | Not started | - |
| 239. Slicing Engine Semantics | 0/2 | Not started | - |
| 240. Notification Stacking And Gizmo Interaction Depth | 0/3 | Not started | - |
| 241. Page Honesty And CLI Surface | 0/2 | Not started | - |
