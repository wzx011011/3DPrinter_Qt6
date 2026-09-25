# Phase 154: QML Preset Diff-View Dialog

**Status:** Ready to execute
**Workstream:** CLOS (PSET-05 closure)
**Requirement:** CLOS-01

## Goal

Wire a QML side-by-side diff dialog that consumes the existing
`PresetServiceMock::comparePresets(A, B)` primitive (shipped Phase 149 as a
`QVariantList` of `{key, valueA, valueB, status}`). The primitive exists; this
phase adds the deferred QML consumer.

## Source-truth mapping

Upstream: `OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp` — diff view mode
renders a 3-column key/valueA/valueB table with added/removed/changed
classification badges. We mirror that layout (key | valueA | valueB + status
badge per row) using the existing Cx controls, no new product behavior.

## Plan

### Wave 1 — C++ wiring (ConfigViewModel proxy)

1. `src/core/viewmodels/ConfigViewModel.h/.cpp`: add
   `Q_INVOKABLE QVariantList comparePresetsDetailed(A, B)` that proxies to
   `presetService_->comparePresets(A, B)` (the structured variant). The legacy
   `QStringList comparePresets` stays untouched (other callers may rely on it).
   Add `Q_INVOKABLE void requestComparePresets()` + `comparePresetsRequired()`
   signal for the SettingsDialog opener pattern (mirrors `requestCreatePreset`).

### Wave 1 — QML dialog

2. `src/qml_gui/dialogs/PresetDiffDialog.qml` (NEW): non-modal
   `ApplicationWindow` with two CxComboBox selectors (A, B) populated from
   `backend.configViewModel.presetNamesByCategory` (existing), a "Compare"
   CxButton, a ListView rendering `comparePresetsDetailed(A,B)` rows as a
   3-column grid with status badges (added = green, removed = red,
   changed = amber), and an empty-state "No differences" placeholder.
3. `src/qml_gui/dialogs/SettingsDialog.qml`: instantiate PresetDiffDialog +
   bind `onComparePresetsRequired` → `dialog.open()`. Add a "Compare presets"
   CxButton next to the existing "Create preset" button.
4. `src/qml_gui/qml.qrc`: register PresetDiffDialog.qml.

### Wave 1 — test anchor

5. `tests/QmlUiAuditTests.cpp`: add a CLOS-01 regression slot
   `v51PresetDiffDialogWired` that asserts:
   - `ConfigViewModel.h` exposes `comparePresetsDetailed` returning QVariantList
   - `PresetDiffDialog.qml` exists and consumes the model roles key/valueA/valueB/status
   - `SettingsDialog.qml` instantiates PresetDiffDialog
   - `qml.qrc` registers PresetDiffDialog.qml
   All via source-text patterns (QVERIFY2 on file contents).

## Verification

- Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0
- 5/5 ctest groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser)
- 12 v5.0 regression slots still pass
- The new `v51PresetDiffDialogWired` slot passes
