# Phase 56 — Plan 03 Summary: QML Settings Dialogs + main.qml Wiring

**Plan:** 56-03 (Wave 3)
**Status:** Complete — all tasks built and tested green
**Date:** 2026-07-03

## Objective

Build the three independent non-modal `ApplicationWindow` settings dialogs
(printer / material / process) parameterized by a single `SettingsDialog.qml`
shell; extract `OptionRow.qml` (typed renderer across all 7 option kinds) and
`GroupNavSidebar.qml` from the existing `ParamsPage.qml` patterns; register
them in `qml.qrc`; wire `main.qml` to dispatch `backend.settingsRequested` to
the correct dialog; flip the Wave 0 RED scaffolds GREEN.

## Key Changes

### Task 1 — `OptionRow.qml` (committed `78ca320`)
Self-contained typed-option renderer. Dispatches by `optType`:
- `bool` → `CxSwitch`
- `int` → `CxSlider` + `CxSpinBox` (with the new `suffix` property for units)
- `double` / `percent` → `CxSlider` + `TextInput` + `DoubleValidator`
  (Qt6 `SpinBox.stepSize` is int-only — `.claude/rules/debugging.md`)
- `enum` → `CxComboBox`
- `string` → `TextArea` (multiline; no `CxTextArea` in the current set)
Exposes `totalHeight` (content height + optional 28px group-header band),
dirty dot, read-only tag, nullable/value-source indicator, per-extruder flag,
tooltip, zebra striping. Cx* controls only; all strings `qsTr()`.

### Task 2 — `SettingsDialog.qml` + `GroupNavSidebar.qml` + `qml.qrc` (committed `f9d8e00`)
- `SettingsDialog.qml`: non-modal `ApplicationWindow` (`Qt.Window |
  Qt.WindowCloseButtonHint`, `Qt.NonModal`, 920×640, min 720×480),
  parameterized by `presetTier`. Six layout regions: title bar (preset combo +
  Modified badge + Save As + close), preset bar (Save / Delete / Reset All +
  compat indicator), tab strip (upstream `Tab.cpp` page names per tier),
  main content (`GroupNavSidebar` + option `ListView` delegating to `OptionRow`),
  search bar (`CxTextField` + Advanced `CxSwitch` + Reset Group + match count),
  footer (Save / Discard / Cancel). Dirty-guarded close via the existing
  `UnsavedChangesDialog`; Esc shortcut; `show()` calls `requestActivate()`.
- `GroupNavSidebar.qml`: left option-group nav with count/dirty badges driven
  by `configVm.groupNames(tier)`; emits `groupSelected`.
- `qml.qrc`: registers `SettingsDialog.qml`, `OptionRow.qml`, `GroupNavSidebar.qml`.
- Old `SettingsPage.qml` / `ParamsPage.qml` / `ConfigPage.qml` / `SearchDialog.qml`
  **untouched** — cleanup is Phase 57.

### Task 3 — `main.qml` wiring + test GREEN flips (committed `f351822`)
- `main.qml`: instantiates three `SettingsDialog` instances (printerSettingsDialog /
  materialSettingsDialog / processSettingsDialog) with the correct tier +
  optionModel bindings; adds `onSettingsRequested(category)` handler dispatching
  to `*.show()`.
- `QmlUiAuditTests`: implemented `settingsDialogUsesOnlyCxControls`,
  `settingsDialogNoRawControls`, `settingsDialogStringsQsTr`,
  `settingsDialogMainQmlDispatchStructural` (all GREEN).
- `ViewModelSmokeTests`: implemented `testConfigOptionModelSevenTypes`
  (all 6 dispatch types present), `testUnsavedChangesGuardOnDirtyClose`
  (`isPresetDirty` true after edit, false after `resetAllGlobalOptions`),
  `testNullableAndVectorOptions` (nullable + vector options across the 3 tiers).
  All GREEN.

## Files

| File | Change |
|------|--------|
| `src/qml_gui/components/OptionRow.qml` | new — typed option renderer |
| `src/qml_gui/components/GroupNavSidebar.qml` | new — group nav sidebar |
| `src/qml_gui/dialogs/SettingsDialog.qml` | new — parameterized dialog shell |
| `src/qml_gui/main.qml` | 3 dialog instances + onSettingsRequested handler |
| `src/qml_gui/qml.qrc` | registers the 3 new files |
| `tests/QmlUiAuditTests.cpp` | 4 settings audit tests implemented |
| `tests/ViewModelSmokeTests.cpp` | 3 Wave 0 scaffolds implemented |

## Self-Check: PASSED

- Incremental build: 185/185, 0 errors.
- `ViewModelSmokeTests`: **85 passed, 0 failed, 1 skipped** (pre-existing
  THUMB-03 unrelated `multiPlate3mfRoundTripPreservesState` skip).
- `QmlUiAuditTests`: **36 passed, 0 failed**.
- Canonical `auto_verify_with_vcvars.ps1`: run for final phase gate
  (result recorded separately).
- No raw `QtQuick.Controls` in the 3 new files (TextArea allowed in OptionRow).
- All user-visible strings `qsTr()`; comments English ASCII-only; UTF-8 no BOM.
- Old SettingsPage/ParamsPage/ConfigPage/SearchDialog untouched.

## Notes / Deviations

- `settingsDialogStringsQsTr` uses a per-file qsTr-count threshold
  (SettingsDialog ≥5; OptionRow/GroupNavSidebar ≥1) instead of a per-literal
  parser — OptionRow/GroupNavSidebar are mostly dynamic bindings. Full
  per-string visual parity is the Phase 58 manual UAT.
- `testNullableAndVectorOptions` scans all 3 tiers (print/filament/machine) for
  nullable/vector options, since nullable/vector options are tier-dependent
  (per-extruder filament temps are vector; inheritable printer options nullable)
  and the print tier alone may have none.
- A latent use-after-move bug in `fuzzyMatch` (pre-existing, exposed when the
  search test first reached it after the Wave-2 filterOptionIndices fix) was
  diagnosed and fixed in commit `08d424f` — see that commit message.

## Requirements Covered

SETTINGS-01 (independent dialogs open from sidebar), SETTINGS-02 (tabs/group
nav per tier), SETTINGS-03 (7 typed controls), SETTINGS-04 (dirty/read-only/
value-source indicators), SETTINGS-05 (Save/Delete/Reset All/footer + dirty
guard), SETTINGS-06 (per-dialog search + Advanced toggle). SETTINGS-07
(integration) is Plan 56-04.
