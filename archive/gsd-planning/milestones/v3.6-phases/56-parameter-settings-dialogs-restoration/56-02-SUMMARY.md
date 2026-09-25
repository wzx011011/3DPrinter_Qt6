---
phase: 56-parameter-settings-dialogs-restoration
plan: 02
subsystem: ui
tags: [config-viewmodel, backend-context, page-group-mapping, qt-test, settings-dialog, libslic3r]

# Dependency graph
requires: [56-01]
provides:
  - ConfigViewModel per-tier/per-group Q_INVOKABLE operations (resetGroup, groupNames, dirtyCountForGroup, optNullable, optIsVector, optSidetext)
  - ConfigViewModel filterOptionIndices per-tier dispatch (printer/filament/print + machine/process legacy aliases)
  - ConfigOptionModel static page-to-group mapping tables for printer/filament/process tiers
  - BackendContext forwardSettingsRequest two-step contract (setActivePresetTier -> emit settingsRequested)
  - 5 Wave 0 RED scaffolds flipped GREEN in ViewModelSmokeTests
affects: [56-03, 56-04]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "normalizedTier() accepts both new tier strings and legacy aliases for backward compat"
    - "optionModelForTier dispatch pattern for all per-tier ConfigViewModel proxies"
    - "Static QHash page-group mapping tables derived from upstream Tab.cpp"

key-files:
  created: []
  modified:
    - src/core/viewmodels/ConfigViewModel.h
    - src/core/viewmodels/ConfigViewModel.cpp
    - src/qml_gui/Models/ConfigOptionModel.cpp
    - src/qml_gui/BackendContext.h
    - src/qml_gui/BackendContext.cpp
    - tests/ViewModelSmokeTests.cpp

key-decisions:
  - "filterOptionIndices removed category-filter-per-row logic (was printOptions only); now dispatches entirely via optionModelForTier(category)"
  - "normalizedTier() explicitly maps both 'process' and 'print' to the print tier, 'machine' and 'printer' to printer tier"
  - "BackendContext forwardSettingsRequest is a flat two-step: setActivePresetTier(category) then emit settingsRequested(category) with no branches"

requirements-completed: [SETTINGS-02, SETTINGS-04, SETTINGS-05, SETTINGS-06]

# Metrics
duration: 40min
completed: 2026-07-03
---

# Phase 56 Plan 2: C++ Viewmodel + Composition-Root Wiring Summary

**ConfigViewModel extended with per-tier/per-group operations (resetGroup, groupNames, dirtyCountForGroup, opt* proxies) and filterOptionIndices per-tier dispatch. Static page-to-group mapping tables built from upstream Tab.cpp. BackendContext forwardSettingsRequest wired with two-step contract. Five Wave 0 RED scaffolds flipped GREEN.**

## Performance

- **Duration:** 40 min
- **Tasks:** 2
- **Files modified:** 6
- **Commits:** 2 (f435917, 3938485)

## Accomplishments

### Task 1: Static page-to-group mapping tables (f435917)
- Added `kPrinterPageGroupMap`, `kFilamentPageGroupMap`, `kPrintPageGroupMap` static structures in ConfigOptionModel.cpp
- Added `assignPageGroupForTier()` method to populate page/group metadata from mapping tables
- Wired assignment into loadMachineSchema, loadFilamentSchema, and loadFromUpstreamSchema
- pageNames()/groupNames() now return upstream-accurate tab/group names (Basic information, Notes, Filament, Cooling, Quality, Speed, etc.)

### Task 2: ConfigViewModel per-tier/group ops + BackendContext forwarder (3938485)
- Added 6 Q_INVOKABLE methods: resetGroup, optNullable, optIsVector, optSidetext, groupNames, dirtyCountForGroup
- Extended normalizedTier() to accept legacy aliases ("machine"->"printer", "process"->"print")
- Refactored filterOptionIndices to dispatch via optionModelForTier(category) instead of hard-wiring printOptions_
- Removed category-per-row filtering (was only for printOptions); now the tier model handles its own data
- Replaced BackendContext "pending Phase 56" no-op with concrete two-step: setActivePresetTier(category) then emit settingsRequested(category)

### Test Scaffolds Flipped GREEN
- testTabsAndGroupNavPerTier: already GREEN from Task 1 (real assertions, no QFAIL)
- testSettingsDialogOpenFromSidebar: GREEN -- asserts forwardSettingsRequest two-step ordering via QSignalSpy
- testPerOptionDirtyAndValueSource: GREEN -- asserts optIsDirty after setValue, valueSourceForKey, dirtyCountForGroup
- testReadonlyBuiltinGating: GREEN -- asserts requestSavePendingChanges returns false for builtin presets
- testSaveSaveAsResetOptionResetGroupResetAll: GREEN -- asserts resetGroup clears dirty flags per group
- testPerDialogSearchAndFourLevelMode: GREEN -- asserts per-tier dispatch, advancedMode filtering, legacy alias equivalence

### Tests Remaining RED (need 56-03 QML / 56-04 integration)
- testConfigOptionModelSevenTypes: needs QML-level rendering verification (56-03)
- testUnsavedChangesGuardOnDirtyClose: needs QML dialog close guard (56-03)
- testNullableAndVectorOptions: needs QML interaction (56-03)

## Deviations from Plan

None - plan executed exactly as written.

## Threat Flags

None identified (local desktop viewmodel extension, no network/auth surface).

## Self-Check: PASSED

- [x] f435917: Task 1 commit exists
- [x] 3938485: Task 2 commit exists
- [x] ConfigViewModel.h declares resetGroup, optNullable, optIsVector, optSidetext, groupNames, dirtyCountForGroup
- [x] filterOptionIndices dispatches via optionModelForTier
- [x] BackendContext forwardSettingsRequest has two-step body (setActivePresetTier then emit)
- [x] "pending Phase 56" log line removed
- [x] 0 C++ compilation errors
- [x] 5 RED scaffolds flipped GREEN in ViewModelSmokeTests
