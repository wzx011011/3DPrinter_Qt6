---
phase: 86
slug: settings-option-sections-and-typed-controls
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-07-08
audited: 2026-07-08
---

# Phase 86 Validation: Settings Option Sections And Typed Controls

## Test Infrastructure

| Property | Value |
|---|---|
| Framework | Qt Test source/QML audits plus ViewModel smoke tests |
| Config file | root `CMakeLists.txt` / CTest |
| Quick run command | `.\build\QmlUiAuditTests.exe settingsOptionRowsRestorePhase86ControlContract` |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| Estimated runtime | targeted seconds; canonical verifier several minutes |

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | Status |
|---|---|---|---|---|---|---|
| 86-01-01 | 01 | 1 | SETCTRL-01, SETCTRL-02, SETCTRL-03 | red source audit | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | green after implementation |
| 86-01-02 | 01 | 1 | SETCTRL-01, SETCTRL-02, SETCTRL-03 | qml source audit | `.\build\QmlUiAuditTests.exe settingsOptionRowsRestorePhase86ControlContract` | green |
| 86-01-03 | 01 | 1 | SETCTRL-01, SETCTRL-02, SETCTRL-03 | regression suite | `.\build\QmlUiAuditTests.exe settingsOptionRowsRestorePhase86ControlContract settingsDialogRestoresPhase85ShellContract settingsDialogMainQmlDispatchStructural settingsDialogReadOnlySaveOpensSaveAs leftSidebarParamsPanelUsesRealOptionRows` | green |
| 86-01-03 | 01 | 1 | SETCTRL-02, SETCTRL-03 | viewmodel smoke | `.\build\ViewModelSmokeTests.exe testSettingsDialogOpenFromSidebar settingsOpenDoesNotInvalidateSliceResults sidebarSettingsForwardEmitsRequestedSignal testPerDialogSearchAndFourLevelMode` | green |
| 86-01-04 | 01 | 1 | SETCTRL-01, SETCTRL-02, SETCTRL-03 | canonical verifier | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | green |

## Requirement Sampling

| Requirement | Coverage | Evidence |
|---|---|---|
| SETCTRL-01 | covered | `OptionRow.qml` section icon rail, divider, compact title, stable delegate sizing, and full QML audit evidence. |
| SETCTRL-02 | covered | `CxCheckBox`, `CxSpinBox`, `CxComboBox`, text/color-like controls, range visuals, units, and `optionModel.setValue` audit tokens. |
| SETCTRL-03 | covered | Fixed `metadataLane` badges for dirty, source, read-only, nullable, vector, and bounds states. |

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|---|---|---|---|
| Pixel-level dialog screenshot comparison | SETCTRL-01, SETCTRL-02, SETCTRL-03 | Owned by final Phase 88 runtime visual evidence; automated SettingsDialog capture is blocked on this Windows session | Open printer/material/process settings in the launched app and inspect row density, badges, and control layout. |

## Validation Sign-Off

- [x] All Phase 86 requirements have automated QML/source or ViewModel smoke coverage.
- [x] Full canonical verifier passed after implementation.
- [x] No LAN/device/cloud/network scope was introduced.
- [x] `nyquist_compliant: true` set in frontmatter.
