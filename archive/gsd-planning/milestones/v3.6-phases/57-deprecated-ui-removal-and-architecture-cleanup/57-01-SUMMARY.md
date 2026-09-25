---
phase: 57-deprecated-ui-removal-and-architecture-cleanup
plan: 01
subsystem: qml-gui, core-viewmodels
tags: [cleanup, deletion, page-router, qml-resources]
requires:
  - 56-03 (SettingsDialog/OptionRow/GroupNavSidebar shipped as the replacement surface)
provides:
  - "7 obsolete QML files removed from disk and qml.qrc"
  - "Dead deferred-config-exit machinery excised from BackendContext"
  - "leave-settings-page action branch removed from ConfigViewModel"
affects:
  - "Pending-unsaved queue for preset/scope actions (retained, still backs Phase 56 dialogs)"
  - "BackendContext page router (simplified setCurrentPage + 3 topbar methods)"
tech-stack:
  added: []
  patterns:
    - "Audit-first deletion discipline (grep zero live refs before delete)"
key-files:
  created: []
  modified:
    - src/qml_gui/qml.qrc
    - src/qml_gui/qmldir
    - src/qml_gui/BackendContext.h
    - src/qml_gui/BackendContext.cpp
    - src/qml_gui/dialogs/SettingsDialog.qml
    - src/qml_gui/panels/LeftSidebar.qml
    - src/qml_gui/components/GroupNavSidebar.qml
    - src/qml_gui/components/OptionRow.qml
    - src/core/viewmodels/ConfigViewModel.h
    - src/core/viewmodels/ConfigViewModel.cpp
    - tests/ViewModelSmokeTests.cpp
  deleted:
    - src/qml_gui/pages/SettingsPage.qml
    - src/qml_gui/pages/ConfigPage.qml
    - src/qml_gui/components/ParamsPage.qml
    - src/qml_gui/components/SearchDialog.qml
    - src/qml_gui/panels/Sidebar.qml
    - src/qml_gui/panels/FilamentPanel.qml
    - src/qml_gui/panels/PrintSettings.qml
decisions:
  - "Treated the entire test method configExternalTransitionDefersAndResumesNavigation as a live caller of the dead machinery (the plan audit only enumerated the leaveOk block at lines 733-737) and removed it; deviation logged below."
metrics:
  duration: ~50min
  completed: 2026-07-03
  tasks: 4
  files_touched: 11 modified + 7 deleted
---

# Phase 57 Plan 01: Deprecated UI Removal and Architecture Cleanup Summary

Removed the 4 off-design Settings files locked by Phase 50 §1.6, the 3 legacy
sidebar panels deferred by Phase 52, and the now-dead ConfigPage-embedding route
machinery in BackendContext/ConfigViewModel. CLEAN-01 fully delivered; the
deletion half of CLEAN-02 delivered; pending-unsaved queue for preset/scope
actions retained.

## What Was Done

### Task 1 — Pre-deletion reference audit (read-only)

Grep across `src/` and `tests/` for every deletion candidate. Verdicts:

| File | Allowed references found | Verdict |
|---|---|---|
| pages/SettingsPage.qml | qml.qrc; self-Loaders to ParamsPage.qml | safe |
| pages/ConfigPage.qml | qml.qrc only | safe |
| components/ParamsPage.qml | qml.qrc, qmldir, SettingsPage.qml, history comments in OptionRow/GroupNavSidebar | safe (comments reworded) |
| components/SearchDialog.qml | qml.qrc, SettingsPage.qml, PrintSettings.qml | safe |
| panels/Sidebar.qml | qml.qrc only | safe |
| panels/FilamentPanel.qml | qml.qrc, Sidebar.qml, self-comment | safe |
| panels/PrintSettings.qml | qml.qrc, Sidebar.qml, self-refs | safe |

Route token call-site counts matched the plan's enumeration exactly. Audit
discrepancy surfaced and resolved — see Deviations.

### Task 2 — Delete 7 QML files + clean qml.qrc/qmldir (commit `36e6dad`)

- Removed 7 `<file>` entries from `src/qml_gui/qml.qrc`.
- Reduced `src/qml_gui/qmldir` to a single Theme singleton line.
- Deleted the 7 QML files (3510 lines removed).
- Reworded 4 stale history comments in LeftSidebar.qml (2), GroupNavSidebar.qml,
  OptionRow.qml, and re-pointed the LeftSidebar edit-preset comment +
  SettingsDialog.qml note to reference the Phase 56 dialog.

### Task 3 — Remove 3 named routes + dead machinery (commit `eb14dde`)

- Removed 5 BackendContext method decls + the `DeferredConfigExitKind` enum +
  3 deferred-state members from BackendContext.h.
- Removed the 2 `connect()` lines in the BackendContext ctor that wired
  ConfigViewModel signals to the now-deleted slots.
- Deleted the impls of `clearDeferredConfigExit`/`executeDeferredConfigExit`/
  `handleConfigPendingActionApplied`/`canLeaveSettingsPage`/
  `requestConfigPageExitIfNeeded` from BackendContext.cpp.
- Simplified `setCurrentPage` to de-dup + set + emit (no ConfigPage exit guard).
- Simplified `topbarNewProject`/`topbarOpenProject`/`topbarImportModel` to drop
  the 3 deferred-exit blocks (latency clock + action + setCurrentPage(1) remain).
- Removed `ConfigViewModel::requestLeaveSettingsPage` decl + impl + the
  `leave-settings-page` branch in `applyPendingAction`.
- Removed the `leaveOk` assertion block from
  `configUnsavedTransitionsQueueAndCancelPendingChanges`.
- Removed the entire `configExternalTransitionDefersAndResumesNavigation` test
  (deviation — see below) and its declaration in the test header section.
- Updated the openSettings comment + SettingsDialog.qml note.

### Task 4 — Canonical build + smoke tests green

`powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
-ExitOnBuildFailure` exited 0. All scene suites passed:

- PrepareScene: passed
- PartPlate: passed
- ViewModelSmokeTests: **84 passed, 0 failed, 1 skipped** (THUMB-03 unchanged;
  count dropped from 85 to 84 because the dead-machinery test method was removed)
- QmlUiAuditTests: **36 passed, 0 failed, 0 skipped** (unchanged)
- PreviewParserTests: passed

Post-deletion grep for the 9 dead tokens
(`canLeaveSettingsPage`, `requestConfigPageExitIfNeeded`,
`executeDeferredConfigExit`, `clearDeferredConfigExit`, `DeferredConfigExitKind`,
`deferredConfigExit`, `handleConfigPendingActionApplied`,
`requestLeaveSettingsPage`, `leave-settings-page`) across `src/` and `tests/`
returns 0 hits.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Removed a second test the plan audit missed**

- **Found during:** Task 1 audit
- **Issue:** The plan's Task 3 audit enumerated only one caller of the dead
  machinery in tests (`tests/ViewModelSmokeTests.cpp:733-737`, the `leaveOk`
  block). The grep surfaced a second test method —
  `configExternalTransitionDefersAndResumesNavigation` at lines 1362-1395 —
  whose body asserted that `BackendContext::setCurrentPage(tp3DEditor)` while
  on `tpProject` with a dirty preset defers navigation and queues
  `leave-settings-page`. That behavior is exactly the dead machinery being
  removed; the test could not compile/pass after Task 3.
- **Fix:** Removed the entire test method body (lines 1356-1389) and its
  declaration at line 155 of `tests/ViewModelSmokeTests.cpp`. This is the
  reason ViewModelSmokeTests drops from 85 to 84 passing.
- **Files modified:** tests/ViewModelSmokeTests.cpp
- **Commit:** eb14dde

**2. [Rule 1 - Bug] Updated an additional stale comment not enumerated by the plan**

- **Found during:** Task 2 grep
- **Issue:** `LeftSidebar.qml:98` had a stale comment "open SettingsPage" and
  `SettingsDialog.qml:8` carried a stale "old SettingsPage/ParamsPage/...are
  NOT removed here; cleanup is Phase 57" note. The plan only authorized
  rewording 3 history comments in LeftSidebar/GroupNavSidebar/OptionRow.
- **Fix:** Updated both to point at the live SettingsDialog surface (Rule 1 —
  comments are not bugs per se, but stale comments referencing deleted files
  would have confused future readers and tripped Plan 57-02's audit grep).
- **Files modified:** src/qml_gui/panels/LeftSidebar.qml,
  src/qml_gui/dialogs/SettingsDialog.qml
- **Commit:** 36e6dad (LeftSidebar), eb14dde (SettingsDialog)

## Verification

- `test ! -f` chain for all 7 deleted files prints PASS_DELETE.
- Grep for 9 dead route/action tokens across `src/` + `tests/` returns 0 hits.
- Canonical `auto_verify_with_vcvars.ps1` exits 0 with all scene suites green.
- Phase 56 replacement dialogs (SettingsDialog/OptionRow/GroupNavSidebar) build
  and pass QmlUiAuditTests 36/0.

## Known Stubs

None. This plan deletes code; it does not introduce any stubbed data paths.

## Threat Flags

None. The phase touches no network, auth, storage, IPC, or external-input
surface (per the plan's threat model, T-57-03/04/SC accepted).

## Self-Check: PASSED

- All 7 deleted QML files confirmed absent on disk via `test ! -f`.
- Both task commits (`36e6dad`, `eb14dde`) present in `git log`.
- Post-deletion grep for 9 dead tokens across `src/` and `tests/`: 0 hits.
- Build green; ViewModelSmokeTests 84/0; QmlUiAuditTests 36/0.
