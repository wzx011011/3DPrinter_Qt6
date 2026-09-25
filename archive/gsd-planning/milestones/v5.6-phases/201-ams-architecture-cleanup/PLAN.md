# Phase 201 Plan: AMS Architecture Cleanup (mock -> ViewModel)

**Milestone:** v5.6
**Scope:** Architecture cleanup only (no network / device / cloud). The mock
data hardcoded in `AMSSettingsDialog.qml` is moved into a C++ ViewModel, and
user edits are persisted to local QSettings so they survive dialog close and
application restart. Real AMS hardware integration is explicitly out of scope.

## Goal

Eliminate the 100%-hardcoded mock QML in `AMSSettingsDialog.qml` by introducing
an `AmsMaterialsViewModel` C++ object that owns the mock AMS slot data and the
mapping-rule catalog. The data source remains mock; the architectural cleanup
value is: (1) UI no longer bakes in literals, (2) edits persist instead of being
lost on dialog close, (3) a clear extension point exists for a future
device-backed ViewModel.

## Background

`AMSSettingsDialog.qml` (lines 22-37 pre-Phase-201) declared every data shape as
`readonly property` literals:
- `slotColors` / `slotNames` / `slotMaterials` / `slotAutoSwap` (4-slot mock)
- `materialTypes` (8-entry catalog)
- `mappingRules` (3-row mock)
- `remainingPct` (4 mock values)

Edit state lived in `editNames` / `editMaterials` / `editAutoSwap` properties
seeded in `onOpened` from the readonly mocks; edits were never written back, so
closing the dialog discarded them. The "add mapping" button was
`enabled: false` (line 266). There was no C++ backend and no persistence.

The dialog is opened via `BackendContext::showAMSSettingsDialog()` (BackendContext.cpp:514)
which emits `showAMSSettingsDialogRequested`; `main.qml` connects that to
`amsSettingsDialog.open()` (main.qml:520). `BackendContext` is the existing
context property that already exposes every other ViewModel
(`editorViewModel`, `monitorViewModel`, ...) as `Q_PROPERTY(QObject*)`.

## Files

- Add: `src/core/viewmodels/AmsMaterialsViewModel.h`
  - `final` QObject with the project's ViewModel conventions (camelCase methods,
    `m_`-prefixed snake_case members, single `stateChanged` NOTIFY for all
    `Q_PROPERTY`s mirroring `EditorViewModel`).
  - `Q_PROPERTY`: `slotCount`, `slotColors` (QVariantList),
    `slotNames` (QStringList), `slotMaterials` (QStringList),
    `slotAutoSwap` (QVariantList bool), `remainingPct` (QVariantList int),
    `materialTypes` (QStringList), `mappingRules` (QVariantList),
    `mappingRuleCount`.
  - `Q_INVOKABLE` edit API: `setSlotName(int,QString)`,
    `setSlotMaterial(int,QString)`, `setSlotAutoSwap(int,bool)`,
    `setRemainingPct(int,int)`, `addMappingRule(int,int,int)`,
    `removeMappingRule(int)`, `resetToDefaults()`.
- Add: `src/core/viewmodels/AmsMaterialsViewModel.cpp`
  - Construct seeds the mock defaults (verbatim from the old QML literals; the
    4 slot colors mirror Theme statusInfo/Error/accent/Warning hex tokens so the
    default visual is byte-identical to pre-Phase-201).
  - `loadFromSettings()` applies persisted overrides if the version sentinel
    matches; `saveToSettings()` writes on every successful edit; `resetToDefaults()`
    clears the `ams/materials/*` keys and re-seeds.
  - Slot-name defaults go through `tr()` to preserve translatability that the old
    QML `qsTr()` had.
- Modify: `src/qml_gui/BackendContext.h`
  - Forward-declare `AmsMaterialsViewModel`.
  - Add `Q_PROPERTY(QObject *amsMaterialsViewModel ...)`, accessor decl, member.
- Modify: `src/qml_gui/BackendContext.cpp`
  - Include the header; construct `new AmsMaterialsViewModel(this)` next to the
    other ViewModels; add the accessor returning it.
- Modify: `src/qml_gui/dialogs/AMSSettingsDialog.qml`
  - Remove all hardcoded `readonly property` mocks + the `edit*` scratch state.
  - Add `property var amsVm: null` and derive every read via `_`-prefixed
    readonly views over the ViewModel.
  - Editing controls call `amsVm.setSlotName/Material/AutoSwap` directly.
  - "Add mapping" button enabled and wired to `amsVm.addMappingRule(...)`.
  - Mapping rows get a per-row delete button -> `amsVm.removeMappingRule(index)`.
  - Footer gains a "Reset defaults" button -> `amsVm.resetToDefaults()`.
  - The `onOpened` initializer that copied mocks into edit state is removed
    (edits now persist through the ViewModel, no snapshot/restore needed).
- Modify: `src/qml_gui/main.qml`
  - Set `amsVm: backend.amsMaterialsViewModel` on the `AMSSettingsDialog`
    instance so the dialog reads/writes through the singleton ViewModel.
- Modify: `CMakeLists.txt`
  - Register `AmsMaterialsViewModel.{h,cpp}` next to the other ViewModels in the
    source list.

## Steps

- [x] Read `AMSSettingsDialog.qml`, `BackendContext.{h,cpp}`, an existing
      ViewModel template (`HomeViewModel`, `MonitorViewModel`), and the CMake
      ViewModel block to lock down conventions.
- [x] Implement `AmsMaterialsViewModel.{h,cpp}`: mock defaults verbatim, versioned
      QSettings persistence under `ams/materials/*`, single-signal NOTIFY.
- [x] Register the ViewModel in `BackendContext` (Q_PROPERTY + accessor + member
      + construction + include), matching the existing `multiMachineViewModel`
      pattern.
- [x] Rewrite `AMSSettingsDialog.qml`: drop all readonly mocks, bind `amsVm`,
      route edits/deletes/add-mapping/reset through the ViewModel.
- [x] Wire `amsVm` in `main.qml` (`backend.amsMaterialsViewModel`).
- [x] Add the two new files to `CMakeLists.txt`.
- [ ] Bracket-balance check on every changed file.

## Key design decisions

1. **Mock data source stays mock.** Per the project decision, this phase is an
   architecture cleanup only; the data is NOT sourced from a real printer, AMS
   device, or cloud service. Real device integration belongs to the
   printer-hardware scope which is not open in v5.6. The cleanup value is that
   the literal data now lives in one C++ owner instead of being scattered across
   QML `readonly property` blocks, and that there is a clear ViewModel seam a
   future device-backed implementation can swap in.

2. **Persistence is the value-add.** Pre-Phase-201, edits lived only in QML
   scratch properties and were lost when the dialog closed. The ViewModel writes
   every successful edit to QSettings (`ams/materials/*`, plus a version
   sentinel `ams/materials/version`). This turns "edits lost on close" into
   "persistent mock". Reset-to-defaults clears the keys so users can recover the
   baseline. The persistence is purely local -- QSettings on disk -- and touches
   no network.

3. **Single-signal NOTIFY (`stateChanged`).** Mirrors `EditorViewModel`: every
   `Q_PROPERTY` carries `NOTIFY stateChanged` and every mutating API emits it
   once after the change + persist. This keeps the QML side simple (one binding
   refresh point) and matches an established project convention rather than
   introducing per-property signals.

4. **Default colors mirrored from Theme tokens as hex literals.** The ViewModel
   cannot reach into QML `Theme.qml`, so the four default slot colors are stored
   as hex literals (`#3b9eff` / `#e04040` / `#18c75e` / `#f5a623`) matching the
   Theme tokens (`statusInfo` / `statusError` / `accent` / `statusWarning`). This
   keeps the default visual identical; if the theme tokens change later, only the
   ViewModel literals need to follow. Documented inline.

5. **Slot-name defaults use C++ `tr()`.** The old QML used `qsTr("蓝色 PLA")`
   etc. Moving the literals to C++ would have lost translatability, so the
   defaults are wrapped in `tr()` -- Qt extracts them into the same `.ts`
   pipeline. (Material-type codes like `PLA` / `ABS` are intentionally NOT
   translated, matching the original QML.)

6. **Mapping add appends a generic rule.** `addMappingRule` takes explicit
   (slot, extruder, temp); the QML "Add mapping" button passes
   `(mappingRuleCount + 1, mappingRuleCount + 1, 210)` as a sensible default so
   the row appears immediately. A full mapping-row editor (in-place temp/slot
   spinboxes) is deferred -- the ViewModel API already supports it.

## Known limitations / deferred

- **Mapping-rule inline editing.** The list renders rows read-only; only add and
  delete are wired. Inline slot/extruder/temp spinboxes calling
  `addMappingRule`/`removeMappingRule` is a follow-up (the ViewModel already
  exposes the API; only the QML editor rows are missing).
- **No real device integration.** Out of scope by design; the ViewModel has no
  signal for device telemetry. A future `AmsDeviceViewModel` can layer on top.
- **Slot count is fixed at 4.** Mirrors upstream AMS (4 slots). Multi-AMS /
  >4-slot devices would need `slotCount` to be configurable and the persistence
  shape to scale; not needed for v5.6 mock cleanup.
- **Color palette is not user-editable.** Only slot name / material / auto-swap /
  remaining-pct / mapping rules are editable; the slot color picker is still a
  visual placeholder (pre-existing behavior preserved).

## Verify

- [ ] Build: `AmsMaterialsViewModel.{h,cpp}` compiles and links; the dialog's
      `amsVm` binding resolves at runtime.
- [ ] Manual: open the AMS dialog; the default mock data matches the
      pre-Phase-201 visual (same 4 colors, names, materials, mappings, percentages).
- [ ] Manual: edit a slot name / material / auto-swap, close and reopen the
      dialog -- the edit survives (QSettings).
- [ ] Manual: click "Add mapping" -- a new row appears; click its "x" -- it is
      removed; both survive close/reopen.
- [ ] Manual: click "Reset defaults" -- all slots/mappings return to the
      original mock values and persisted overrides are cleared.
- [ ] Manual: restart the app -- persisted edits are still present.
