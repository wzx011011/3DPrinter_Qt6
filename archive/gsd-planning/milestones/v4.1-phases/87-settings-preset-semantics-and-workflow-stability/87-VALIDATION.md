---
phase: 87
slug: settings-preset-semantics-and-workflow-stability
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-07-08
audited: 2026-07-08
---

# Phase 87 Validation: Settings Preset Semantics And Workflow Stability

## Test Infrastructure

| Property | Value |
|---|---|
| Framework | Qt Test QML source audits plus ViewModel smoke tests |
| Config file | root `CMakeLists.txt` / CTest |
| Quick run command | `.\build\QmlUiAuditTests.exe settingsDialogDirtyPendingActionsOpenUnsavedGuard` |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| Estimated runtime | targeted seconds; canonical verifier several minutes |

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | Status |
|---|---|---|---|---|---|---|
| 87-01-01 | 01 | 1 | SETSEM-01 | red source/behavior audit | `.\build\QmlUiAuditTests.exe settingsDialogDirtyPendingActionsOpenUnsavedGuard` | green after implementation |
| 87-01-01 | 01 | 1 | SETSEM-01, SETSEM-03 | viewmodel smoke | `.\build\ViewModelSmokeTests.exe configUnsavedTransitionsQueueAndCancelPendingChanges configDiscardAppliesPendingTransitionAndRestoresValues configWritableSaveAppliesPendingTransition configReadOnlySaveAsAppliesPendingTransition` | green |
| 87-01-02 | 01 | 1 | SETSEM-01 | qml dirty guard wiring | `.\build\QmlUiAuditTests.exe settingsDialogDirtyPendingActionsOpenUnsavedGuard settingsDialogReadOnlySaveOpensSaveAs` | green |
| 87-01-03 | 01 | 1 | SETSEM-02, SETSEM-03 | regression smoke | `.\build\ViewModelSmokeTests.exe settingsOpenDoesNotInvalidateSliceResults sidebarPresetChangeInvalidatesSliceResults` | green |
| 87-01-04 | 01 | 1 | SETSEM-01, SETSEM-02, SETSEM-03 | canonical verifier | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | green |

## Requirement Sampling

| Requirement | Coverage | Evidence |
|---|---|---|
| SETSEM-01 | covered | Pending preset save, discard, cancel, read-only Save As, and close guard paths are covered by QML audit and ViewModel smoke tests. |
| SETSEM-02 | covered | Per-dialog search/filter behavior remains covered by existing smoke tests; dirty/error indicators remain outside the filtered option rows. |
| SETSEM-03 | covered | Slice invalidation and Prepare/Preview payload stability are covered by `settingsOpenDoesNotInvalidateSliceResults` and `sidebarPresetChangeInvalidatesSliceResults`. |

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|---|---|---|---|
| Human click-through of dirty guard dialog wording | SETSEM-01 | Automated tests verify wiring and state transitions; visual wording is manually inspected in the running app | Make a preset dirty, switch presets, then choose Cancel, Discard, and Save/Save As flows. |

## Validation Sign-Off

- [x] All Phase 87 requirements have automated source or ViewModel behavior checks.
- [x] Full canonical verifier passed after implementation.
- [x] No LAN/device/cloud/network scope was introduced.
- [x] `nyquist_compliant: true` set in frontmatter.
