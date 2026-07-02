# Phase 56: Parameter Settings Dialogs Restoration - Context

**Gathered:** 2026-07-02
**Status:** Ready for planning
**Mode:** Smart discuss (autonomous) — all grey areas accepted with recommendations

<domain>
## Phase Boundary

Phase 56 restores printer, material, and process settings as **independent
OrcaSlicer-like settings workflows** (three separate non-modal dialogs), driven
by real `print_config_def` typed option models, with full dirty / save / reset /
search / validation semantics and Prepare + slice integration.

The visual/layout contract is **frozen by Phase 50** (`50-INVENTORY.md`,
`docs/v3.6-ui-inventory.md`): 8 printer regions (`SETPRINT-SHELL..DIRTY`) and
8 material regions (`SETMAT-SHELL..DIRTY`), plus process settings by upstream
parity (no process screenshot supplied). Region IDs are immutable; Phase 56
implements them, it does not re-design them.

**In scope (SETTINGS-01..07):**
- Build new independent printer, material, and process settings dialogs
  (`SETPRINT-SHELL`, `SETMAT-SHELL` = Missing; process shell by parity).
- Inside each dialog: preset selector bar, top category tabs, left option-group
  navigation, main typed-option editing area, search + basic/advanced toggle,
  footer Save/Discard/Cancel, dirty/compat/warning indicators.
- C++ typed option-model coverage for boolean, numeric (int/float), enum,
  string, percent, nullable (inherit), and multi-value fields.
- Dirty state, modified-option indicators, value-source / inheritance,
  read-only state (builtin presets), validation warnings, blocking errors.
- Save, Save As, reset-option, reset-group, reset-all, discard, cancel, and
  unsaved-change behavior.
- Search and basic/advanced filtering with filtered/no-match states.
- Integration with the Prepare sidebar entry point, slice invalidation, merged
  slicing config, and project save/restore.

**Out of scope:**
- Removal of the replaced embedded `SettingsPage.qml` / `ConfigPage.qml` /
  `ParamsPage.qml` / `SearchDialog.qml` + their qrc/routes/imports → that
  removal is **Phase 57** (Deprecated UI Removal). Phase 56 builds the new
  dialogs alongside; it does not delete the old surface yet (the cleanup
  checklist is already locked in `50-INVENTORY.md` §1.6).
- Device send / cloud print / Monitor job lifecycle.
- AssembleView, auto filament-map, wipe-tower rendering.
- Changing libslic3r slicing algorithms.

**Reconciliation note (from `50-INVENTORY.md` §2 Observation):** the per-region
§1.3/§1.4 tables mark the 14 non-shell Settings sub-regions as `modify` while
the §1.5 summary marks them `replace`. §1.5 is the decision-of-record: these
sub-regions are **rebuilt inside the new dialogs** (replace), not patched in
place. Phase 56 plans should treat all 14 as `replace` and the plan-phase may
update §1.3/§1.4 cells to `replace` to remove the discrepancy.

</domain>

<decisions>
## Implementation Decisions

### Dialog Architecture
- **Shape:** three independent, **non-modal** settings dialogs (printer,
  material, process) — each a separately-focusable window the user can keep open
  while interacting with Prepare, matching upstream `Tab.cpp`. Not modal popups,
  not embedded pages (SETTINGS-01 rejects the Project/Settings embedding).
- **Separate dialogs, not one combined:** one dialog per PresetCollection scope,
  matching the separate `SETPRINT-SHELL` / `SETMAT-SHELL` Missing inventory rows
  and upstream `create_preset_tab` per category. Reject the single-combined-tab
  approach (current `SettingsPage.qml` tier bar is the off-design pattern being
  replaced).
- **Open path:** opened from the Prepare left-sidebar per-section "Setting"
  button via the existing `settingsRequested(category)` signal that Phase 52
  wired as a logged no-op. BackendContext forwards it to open the correct
  dialog scoped to the active category. Process settings open from the process
  section's "Setting" button (Phase 52 made it visible+enabled).
- **Close path:** X button or Esc. On close/tab-switch/preset-switch when the
  current preset is dirty → show `UnsavedChangesDialog` (save / discard /
  cancel) — do NOT auto-discard.
- **Process settings (no screenshot):** build the third process-settings dialog
  by upstream `Tab.cpp` print-process parity (Quality / Strength / Speed /
  Support / Cooling / Infill / Skirt / Advanced / G-code groups), using the same
  shell, option-group nav, typed option model, and save/reset semantics as
  printer and material.

### Option Model Coverage (SETTINGS-03)
- **Option list source:** driven from the real upstream
  `Slic3r::print_config_def` (libslic3r), filtered by settings category and by
  basic/advanced level. No hand-curated screenshot-only subset (Preset Rule).
- **Typed-control coverage:** all seven option types in one pass — boolean,
  integer, float (numeric), enum (combo), string, percent + unit, nullable
  (inherit), and multi-value (per-extruder) fields. `ConfigOptionModel`
  scaffolding already exists; expand it to cover all types.
- **Units:** read from `print_config_def` option metadata (`sid` / unit), rendered
  as a suffix on the control (mm, %, °C, s, mm/s). Not hardcoded.
- **Null / inheritance:** a null option value means **inherit-from-parent-preset**.
  The UI shows the inherited value with a value-source indicator (SETTINGS-04);
  editing overrides locally, resetting returns to inherited.

### Dirty / Save / Reset Semantics (SETTINGS-04, SETTINGS-05)
- **Dirty granularity:** per-option dirty + per-preset global dirty, with inline
  modified-option markers. Mirrors upstream `Tab.cpp` optcache diff against the
  parent preset.
- **Value source / read-only:** surface inherited-vs-overridden source, and
  gate builtin presets honestly read-only via `PresetServiceMock::isBuiltinPreset`
  (rename/delete/edit already gated in legacy `PrintSettings.qml`; carry the
  same rule into the new dialogs).
- **Reset levels:** three — reset-option (to inherited value), reset-group (all
  options in the visible group), reset-all (entire preset to defaults). All
  three must match upstream.
- **Save vs Save As:** **Save** overwrites the current user preset; if the
  preset is builtin, Save is blocked and forces **Save As** via the existing
  `SavePresetDialog.qml`. Mirrors upstream `Tab::save_preset`.
- **Unsaved-change guard:** the existing `UnsavedChangesDialog.qml` guards
  close, preset-switch, and category-switch when dirty (save / discard / cancel).

### Search / Filter + Integration (SETTINGS-06, SETTINGS-07)
- **Search scope:** per-dialog (matches upstream per-tab search). The existing
  `SearchDialog.qml` + `ConfigViewModel::searchResultPage(optionIndex)` jump
  stays available; each dialog's search filters its own category's option model.
  Filtered / no-match states must be useful and source-truth-aligned.
- **Basic/Advanced toggle:** driven from `print_config_def` option `level`
  (advanced vs basic), matching upstream `m_mode`. Not hardcoded per-option.
- **Slice invalidation:** reuse the Phase-52 connect
  (`ConfigViewModel::stateChanged` → `EditorViewModel::invalidateSliceResultsForAllPlates`)
  already wired in BackendContext. Settings edits flow through the same path —
  any option edit or preset change invalidates stale slice results across all
  plates.
- **Project save/restore:** dirty user-overrides persist via the existing 3MF
  scoped-config-override path in `ProjectServiceMock` (per-plate / per-object
  scoped overrides already modeled). Users are NOT forced to save-as-preset
  before a project save.

### Claude's Discretion
- Exact QML dialog component decomposition (one `SettingsDialog.qml` shell
  parameterized by category vs three near-identical files) — prefer a single
  parameterized shell + per-category category/group model injection to avoid
  triplication, but keep the three PresetCollection scopes strictly separate in
  the C++ backend.
- Exact `ConfigOptionModel` extension shape (role-based vs type-based dispatch)
  for the 7 option types, as long as all 7 are covered and units/source/null are
  surfaced.
- Whether `PresetListModel` becomes the canonical model for the new dialogs
  (Phase 52 left it under-wired for 56) or `ConfigOptionModel` is extended —
  pick whichever yields the cleanest typed-control coverage.
- Signal/slot wiring for `settingsRequested(category)` → dialog open in
  BackendContext (the composition root owns both).
- Test file placement (extend `ViewModelSmokeTests.cpp` / `E2EWorkflowTests.cpp`
  vs a new `SettingsDialogTests.cpp`).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/viewmodels/ConfigViewModel.h` (247 lines) — preset Q_PROPERTYs
  (`printerPresetNames`, `filamentPresetNames`, `printPresetNames`,
  `currentPrinter/Filament/PrintPreset`, `currentPresetCombinationValid`,
  `isPresetDirty`, `globalModifiedCount`, `settingsScope/TargetType/TargetName`,
  `activePresetTier`), `requestCurrent*Preset` pending-unsaved setters,
  `filterOptionIndices` (search), `scopeSubtitle()`, `searchResultPage()`.
- `src/core/services/PresetServiceMock.h` — `PrintCat/FilamentCat/PrinterCat`
  category enum, preset CRUD, compatibility (`isFilamentCompatibleWithPrinter`,
  `isCurrentSelectionCompatible`, `currentSelectionCompatibilityMessage`),
  `isBuiltinPreset` gating, `findCompatibleFilament`. All state Phase 56 needs.
- `src/qml_gui/Models/ConfigOptionModel.h` — typed option model scaffolding;
  expand to bool/int/float/enum/string/percent/nullable/multi-value.
- `src/qml_gui/Models/PresetListModel.{h,cpp}` — exists but under-wired; Phase
  52 explicitly left it for 56.
- `src/qml_gui/dialogs/SavePresetDialog.qml`, `UnsavedChangesDialog.qml`,
  `SearchDialog.qml`, `ExportPresetBundleDialog.qml` — all exist and are
  reusable (Save As flow, unsaved guard, search jump, bundle export).
- `src/qml_gui/components/ParamsPage.qml` — current (off-design) category-list +
  option renderer; reference for layout density but **replaced**, not reused as
  the host (per §1.5 decision-of-record).
- `src/core/services/ProjectServiceMock` — scoped config overrides path for
  SETTINGS-07 persistence.

### Established Patterns
- C++ services/viewmodels hold all durable state; QML is presentation + wiring
  (`.claude/rules/qml-boundaries.md`).
- ViewModels expose state via `Q_PROPERTY` (READ + NOTIFY); actions via
  `Q_INVOKABLE`; single bulk `stateChanged()` where practical.
- BackendContext is the composition root — owns all viewmodels + services,
  wires cross-vm connects (e.g., the Phase-52 `configVm → editorVm`
  invalidation connect lives here). The `settingsRequested(category)` →
  dialog-open forward belongs here.
- Custom controls are `Cx*` (`CxButton`, `CxComboBox`, `CxSlider`, `CxSpinBox`,
  `CxTextField`, `CxCheckBox`, …); no raw QtQuick.Controls in consuming code
  (v14-01 component-system gate).
- Theme tokens (`Theme.bgBase`, `Theme.accent`, `Theme.spacingMD`, …); all
  user-visible strings via `qsTr()`.
- Comments English + ASCII-only; UTF-8 without BOM.

### Integration Points
- `BackendContext` — forward `settingsRequested(category)` to open the matching
  independent dialog scoped to the active category (Phase 52 left the no-op log).
- `ConfigViewModel` — extend with per-option dirty / value-source / reset
  (`resetOption`/`resetGroup`/`resetAll`) / save-vs-saveas gating if not present.
- `EditorViewModel` — the Phase-52 `stalePlateIndices` / `hasStaleSliceResults`
  properties already surface invalidation; settings edits reuse this path.
- `qml.qrc` — register the new dialog component(s).
- `main.qml` / page router — the new dialogs are independent windows opened on
  demand, NOT a new StackLayout page (SETTINGS-01).

### Screenshots (visual truth)
- `shotScreen/打印机参数设置页.png` — printer settings: shell, preset bar,
  top category tabs (Machine / Extruder), left group nav, typed option area,
  search + advanced toggle, footer actions, dirty/compat indicators.
- `shotScreen/材料参数设置页.png` — material settings: same shell/regions,
  tabs Filament / Temperature / Cooling / Retraction / Advanced / G-code.
- (No process screenshot — upstream parity drives the process dialog.)

</code_context>

<specifics>
## Specific Ideas

- The biggest scope lever is the option-list source: driving from real
  `print_config_def` (accepted) means the dialogs automatically inherit every
  upstream option instead of a hand-curated subset that drifts. This is the
  Preset Rule applied literally.
- The dirty → slice-invalidation path is already wired (Phase 52). Phase 56's
  job is to make sure option edits inside the new dialogs flow through
  `ConfigViewModel` so the existing connect fires — not to build a new hook.
- Honest read-only gating of builtin presets matters: a user must not silently
  "edit" a builtin printer preset and believe it saved. Either block + force
  Save As, or show read-only with an explicit "Save As to modify" affordance.
- Process settings being unscreened is fine BECAUSE upstream `Tab.cpp` is the
  behavior truth — same shell, same groups upstream exposes. The screenshot
  rule applies to printer/material; upstream parity applies to process.
- The single-parameterized-shell-vs-three-files QML choice is Claude's
  discretion, but the three PresetCollection scopes must stay separate in C++.

</specifics>

<deferred>
## Deferred Ideas

- Removal of the replaced embedded surface (`SettingsPage.qml`, `ConfigPage.qml`,
  `ParamsPage.qml`, `SearchDialog.qml` + qrc/routes/imports) → Phase 57
  (cleanup checklist already locked in `50-INVENTORY.md` §1.6).
- Filament color metadata wiring (the FilamentSlot color picker Phase 52 hid) —
  evaluate whether the material dialog now owns it or it stays a sidebar concern.
- "Compare presets" and per-object advanced-settings overrides inside the dialog
  — Phase 52 deferred the Advanced/Compare buttons; reassess scope after the
  core dialogs land.
- Cross-category global search (single search across all three dialogs) —
  current decision is per-dialog search; revisit if UX demands it.
- Full hardware-calibration completion beyond preset read/write — Future.

</deferred>
