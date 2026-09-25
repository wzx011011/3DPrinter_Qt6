---
phase: 88
slug: settings-verification-and-cleanup
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-07-08
audited: 2026-07-08
---

# Phase 88 Validation: Settings Verification And Cleanup

## Test Infrastructure

| Property | Value |
|---|---|
| Framework | Qt Test QML source audits, canonical verifier, runtime visual evidence |
| Config file | root `CMakeLists.txt` / CTest |
| Quick run command | `.\build\QmlUiAuditTests.exe settingsRestorationMilestoneHasFinalVerificationCoverage` |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| Estimated runtime | targeted seconds; canonical verifier several minutes |

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | Status |
|---|---|---|---|---|---|---|
| 88-01-01 | 01 | 1 | SETCLEAN-01, SETVERIFY-01 | red final audit | `.\build\QmlUiAuditTests.exe settingsRestorationMilestoneHasFinalVerificationCoverage` | green after cleanup |
| 88-01-02 | 01 | 1 | SETCLEAN-01 | qml resource/path audit | `.\build\QmlUiAuditTests.exe settingsRestorationMilestoneHasFinalVerificationCoverage deletedSettingsPathsStayAbsent deletedRoutesStayAbsent` | green |
| 88-01-03 | 01 | 1 | SETVERIFY-02 | runtime evidence | runtime screenshots under `visual-evidence/` plus app launch | green with manual SettingsDialog caveat |
| 88-01-04 | 01 | 1 | SETCLEAN-01, SETVERIFY-01, SETVERIFY-02 | canonical verifier | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | green |

## Requirement Sampling

| Requirement | Coverage | Evidence |
|---|---|---|
| SETCLEAN-01 | covered | Final QML audit locks normalized restored settings resources and deleted route/path absence. |
| SETVERIFY-01 | covered | Final QML audit anchors Phase 85 shell, Phase 86 option rows, Phase 87 dirty guard, and settings dispatch. |
| SETVERIFY-02 | covered | Canonical verifier passed, app launch succeeded, and runtime screenshots were captured under `visual-evidence/`. |

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|---|---|---|---|
| Direct printer/material/process SettingsDialog screenshot capture | SETVERIFY-02 | Computer Use screenshot capture failed in this Windows session with `SetIsBorderRequired failed: 0x80004002` | Use the launched app to open printer, material, and process settings dialogs and visually inspect screenshot parity. |

## Validation Sign-Off

- [x] Final QML cleanup and dispatch audits are automated.
- [x] Full canonical verifier passed.
- [x] Runtime app and settings-panel screenshots are recorded.
- [x] Direct SettingsDialog visual capture is explicitly manual-only due Windows capture API failure.
- [x] No LAN/device/cloud/network scope was introduced.
- [x] `nyquist_compliant: true` set in frontmatter.
