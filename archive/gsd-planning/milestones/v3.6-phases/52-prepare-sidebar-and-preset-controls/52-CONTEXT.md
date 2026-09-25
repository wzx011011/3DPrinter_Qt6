# Phase 52: Prepare Sidebar and Preset Controls - Context

**Gathered:** 2026-07-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 52 restores the Prepare left sidebar as the main configuration entry
point. Scope is sidebar-only: the printer/filament/process preset combos,
preset state indicators (compatibility/dirty/read-only), the settings entry
points, the search box, the scope toggles, and — critically — wiring preset
changes to slice invalidation.

Out of scope: settings dialog internals (Phase 56), object list / plate
workflow / viewport (Phase 53), deprecated page removal (Phase 57), and the
full option-list rendering inside ParamsPanel (Phase 56).

The active sidebar is `src/qml_gui/panels/LeftSidebar.qml` (rendered via
DockableSidebar). The legacy `src/qml_gui/panels/Sidebar.qml` (452 lines,
embeds FilamentPanel/PrintSettings) is NOT wired into PreparePage and is
deferred to Phase 57.

</domain>

<decisions>
## Implementation Decisions

### Sidebar Component Reconciliation
- `LeftSidebar.qml` is the active PREP-SIDEBAR Qt target (rendered via
  DockableSidebar from PreparePage.qml:1568). Keep it. Document the legacy
  `Sidebar.qml` (452 lines, not wired into PreparePage) as legacy and defer
  its removal to Phase 57 — it embeds FilamentPanel.qml and PrintSettings.qml
  which may inform Phase 56 settings work, so do NOT remove them in Phase 52.
- Keep the flat QStringList combo binding for the printer/filament/process
  combos (the combos bind `configVm.printerPresetNames` /
  `filamentPresetNames` / etc directly, and this works). Do NOT rewire the
  sidebar combos to `PresetListModel` in Phase 52 — `PresetListModel` is
  under-wired and is more useful for the Phase 56 settings dialogs. Leave it
  for Phase 56.
- Bind the filament slot count in LeftSidebar to `editorVm.extruderCount`
  (dynamic) instead of the hard-coded `Repeater { model: 5 }` at
  LeftSidebar.qml:181-189. This is a real parity fix for PREPSB-01 (the
  upstream sidebar shows one slot per extruder). The per-slot `FilamentSlot`
  component is reused as-is.
- The filament color picker popup in `FilamentSlot.qml` is currently
  visual-only (selecting a color does nothing). For Phase 52: HIDE the
  non-functional color picker popup (it is not screenshot-visible as a
  distinct control) and add an English ASCII TODO comment referencing the
  real filament color metadata path. Do NOT leave silent dead UI. Wiring
  full color metadata is Phase 56 (settings) territory.

### Preset State and Compatibility (PREPSB-03)
- Surface the existing preset states in the sidebar, replacing any placeholder
  text with real C++ bindings:
  - **Compatibility dot**: already in FilamentSlot via
    `configVm.isFilamentCompatible(presetName)` — verify it renders.
  - **Dirty indicator**: ConfigViewModel already exposes `isPresetDirty` and
    `globalModifiedCount`. Surface a dirty dot on each preset combo row
    (printer/filament/process) when the corresponding preset is dirty.
  - **Read-only / builtin**: `PresetServiceMock::isBuiltinPreset` already
    gates rename/delete in PrintSettings.qml — mirror this gating in
    LeftSidebar's preset action buttons (the "✎" edit and any rename/delete
    affordances) so builtin presets are honestly read-only.
- The "Advanced/Compare/Setting" buttons in the process section
  (LeftSidebar.qml:253-283) are currently `visible:false enabled:false`
  stubs. For Phase 52: make the "Setting ☰" button visible AND enabled,
  emitting a `settingsRequested("process")` signal that BackendContext
  forwards (Phase 56 wires the actual independent dialog). Keep "Advanced"
  and "Compare" hidden — they are settings-dialog features (Phase 56). Add
  a clear English ASCII comment marking them as Phase 56 scope.
- Wire the search box (LeftSidebar.qml:288-324, currently empty
  `onAccepted`) to `configVm.filterOptionIndices("", searchText,
  advancedEnabled)` so the search is functional even though the full options
  rendering is Phase 56. The search filters the config option model; the
  ParamsPanel placeholder remains until Phase 56 but the search input is
  live.
- Verify the settings scope toggles (Global/Object/Plate CxButtons at
  LeftSidebar.qml process section) correctly call
  `configVm.requestGlobalScope` / `requestObjectScope` /
  `requestPlateScope` and that `scopeSubtitle()` updates. If working, add a
  test; if broken, wire them.

### Slice Invalidation Integration (PREPSB-05) — CRITICAL GAP FIX
- Wire `ConfigViewModel::stateChanged` (filtered to preset/scope/option
  changes, not every pure-UI-refresh emit) to
  `EditorViewModel::invalidateSliceResultsForAllPlates()` (or a new
  preset-specific invalidator). The connect happens in BackendContext (it
  owns both viewmodels and is the composition root). This closes the
  critical gap where changing a preset did not invalidate a previously-
  sliced/previewed/exported result.
- Invalidation fires on: any preset selection change (printer/filament/
  process), any settings-scope change, and any option edit (dirty). To
  avoid invalidating on no-op re-selections, ConfigViewModel should carry a
  "reason" or the connect should diff the relevant preset/scope before
  invalidating. Conservative default: invalidate on any configVm.stateChanged
  that was not caused by a pure list-refresh.
- Expose staleness to QML: add `Q_PROPERTY(QVariantList stalePlateIndices
  READ stalePlateIndices NOTIFY stateChanged)` and `Q_PROPERTY(bool
  hasStaleSliceResults READ hasStaleSliceResults NOTIFY stateChanged)` on
  EditorViewModel (backed by the existing `m_stalePlateIndices` set at
  EditorViewModel.h:759). Preview/Export UI binds to these to show a
  "stale — reslice" indicator and to disable export of stale results.
- Keep `canSlice` (Phase 51 shell gate) SEPARATE from staleness. canSlice
  reflects "is there something to slice"; staleness reflects "is the
  existing slice result out of date". Do not conflate. The StatusBar /
  preview can show a staleness chip; canSlice stays as-is.

### Scope Boundary and Verification
- Scope boundary: Phase 52 is SIDEBAR-ONLY. It does NOT touch settings
  dialog internals (Phase 56), object list / plate workflow (Phase 53),
  viewport (Phase 53), or deprecated page removal (Phase 57). The only
  files modified are: LeftSidebar.qml, FilamentSlot.qml, BackendContext
  (for the invalidation connect), EditorViewModel (for the staleness
  Q_PROPERTYs), ConfigViewModel (only if a "reason" field is needed), and
  test files.
- The ParamsPanel section in LeftSidebar (lines 520-590, placeholder
  "参数列表暂不可用") is LEFT for Phase 56 (settings option rendering).
  Phase 52's search-box wiring makes the search functional, but full option
  rendering is settings-dialog work. Add a clear comment that Phase 56
  replaces this placeholder.
- Settings entry point interim UX: the "Setting" button emits
  `settingsRequested("process")`; BackendContext forwards it. Since no
  independent dialog exists yet (Phase 56), the forward is a no-op that
  logs an honest "settings dialog pending Phase 56" qInfo message. The
  button is visible (entry point exists) but not yet functional until
  Phase 56. This is acceptable and honest for Phase 52.
- Verification: automated — C++ test for slice invalidation (change a
  preset → stalePlateIndices updates → hasStaleSliceResults true), C++ test
  for compatibility/dirty state surfacing, build via sanitized PATH
  (document the vcvars64.bat + VMware PATH environment issue carried from
  Phase 51), QML audit for sidebar bindings (preset combos, dirty dots,
  search box, scope toggles). Manual visual UAT against the PREP-SIDEBAR
  screenshot region is deferred to Phase 58.

### Claude's Discretion
- Exact signal/slot signature for the configVm → editorVm invalidation
  connect (a direct signal, a lambda with diff, or a new
  `configPresetChanged()` signal on ConfigViewModel).
- Whether to add a new `invalidateSliceResultsForPresetChange()` method on
  EditorViewModel (clearer intent) or reuse
  `invalidateSliceResultsForAllPlates()`.
- Exact dirty-dot visual (a small colored circle, an asterisk, etc.) as
  long as it matches the screenshot density.
- Test file placement (extend ViewModelSmokeTests.cpp vs a new
  SidebarPresetTests.cpp).
- Whether the "reason" field on ConfigViewModel::stateChanged is a QString,
  an enum, or derived from a separate `presetChangeReason()` getter.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/panels/LeftSidebar.qml` (624 lines) — active sidebar, 8
  CollapsibleSections (Printer, Filament, Process, Search, Objects,
  ObjectSettings, ObjectLayers, ParamsPanel). The structure is screenshot-
  aligned; Phase 52 wires the stubs and fixes the slot count.
- `src/qml_gui/components/FilamentSlot.qml` (137 lines) — per-extruder
  slot. Reused as-is except hiding the dead color picker popup.
- `src/qml_gui/Models/PresetListModel.{h,cpp}` — exists but under-wired.
  Left for Phase 56.
- `src/core/services/PresetServiceMock.h` — category enum
  (PrintCat/FilamentCat/PrinterCat), preset CRUD, compatibility
  (`isFilamentCompatibleWithPrinter`, `isCurrentSelectionCompatible`,
  `currentSelectionCompatibilityMessage`), `isBuiltinPreset` gating,
  `findCompatibleFilament` auto-match. All the state Phase 52 needs is here.
- `src/core/viewmodels/ConfigViewModel.h` (247 lines) — preset Q_PROPERTYs
  (printerPresetNames, filamentPresetNames, printPresetNames,
  compatibleFilamentPresetNames, currentPrinter/Filament/PrintPreset,
  currentPresetCombinationValid, currentPresetCompatibilityMessage,
  isPresetDirty, globalModifiedCount, settingsScope/TargetType/TargetName,
  activePresetTier), request setters through pending-unsaved queue,
  `filterOptionIndices` for search, `scopeSubtitle()`.
- `src/core/viewmodels/EditorViewModel.h:696-698,758-759` — slice
  invalidation API (`invalidateSliceResultsForCurrentPlate/Plate/All`),
  `m_slicedPlateIndices`, `m_stalePlateIndices`. The invalidator exists but
  has NO preset-change caller today.

### Established Patterns
- Sidebar combos use `CxComboBox` binding flat QStringLists from ConfigViewModel.
- Preset selection goes through `configVm.requestCurrent*Preset` (queues a
  pending-unsaved action).
- BackendContext owns both EditorViewModel and ConfigViewModel (composition
  root) — the natural place for the invalidation connect.
- Dirty/compat state already has C++ backing; Phase 52 surfaces it in QML.
- Phase 51 established the `stateChanged` signal pattern on BackendContext
  and the connect-from-viewmodel pattern (BackendContext.cpp:80-89).

### Integration Points
- New connect in BackendContext constructor (after the Phase 51
  viewmodel-stateChanged forwards): `configVm.stateChanged` (filtered) →
  `editorVm.invalidateSliceResultsForAllPlates()` + emit
  `editorVm.stateChanged`.
- New Q_PROPERTYs on EditorViewModel: `stalePlateIndices` (QVariantList),
  `hasStaleSliceResults` (bool), NOTIFY `stateChanged`.
- LeftSidebar.qml: bind filament Repeater model to `editorVm.extruderCount`;
  add dirty dots to preset rows; wire search box; make "Setting" button
  visible+enabled emitting `settingsRequested("process")`; verify scope
  toggles.
- FilamentSlot.qml: hide the dead color picker popup + TODO comment.
- BackendContext: forward `settingsRequested(category)` signal (Phase 56
  connects the dialog). Interim: qInfo log "settings dialog pending Phase 56".

</code_context>

<specifics>
## Specific Ideas

- The preset-change → slice-invalidation gap is the most important fix in
  Phase 52 (PREPSB-05). Without it, a user could change a filament preset
  and export G-code based on the OLD preset — a silent correctness bug.
- The hard-coded 5 filament slots is a real parity bug (upstream shows one
  slot per extruder via extruderCount). Fix it by binding the Repeater to
  editorVm.extruderCount.
- Hiding the dead color picker popup is about honest UI — a control that
  looks interactive but does nothing violates the "no silent dead UI"
  principle. Hide it with a TODO rather than leaving it deceptive.
- The "Setting" button interim no-op is acceptable BECAUSE it's honest
  (logged) and the entry point exists for Phase 56 to connect. This is
  different from silent dead UI — the button's destination is explicitly
  deferred, not hidden.
- Slice invalidation must invalidate ALL plates (a preset change affects
  every plate's slice result), not just the current plate.

</specifics>

<deferred>
## Deferred Ideas

- Full filament color metadata wiring (color picker popup) — Phase 56
  (settings) territory, needs real preset color data path.
- The full ParamsPanel option-list rendering — Phase 56 (settings dialogs).
- Advanced/Compare buttons in the process section — Phase 56 (settings
  dialog features).
- Removal of legacy Sidebar.qml / FilamentPanel.qml / PrintSettings.qml —
  Phase 57 (Deprecated UI Removal). They may inform Phase 56 work, so keep
  them until then.
- Rewiring sidebar combos to PresetListModel — Phase 56 (settings dialogs
  benefit more from the structured model).
- Independent settings dialog (the destination of settingsRequested) —
  Phase 56.
- Filament merge/delete context-menu features (FilamentPanel has them,
  LeftSidebar does not) — evaluate in Phase 56 or a later sidebar-polish
  phase.

</deferred>
