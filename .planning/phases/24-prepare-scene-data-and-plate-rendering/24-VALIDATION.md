---
phase: 24
slug: prepare-scene-data-and-plate-rendering
status: approved
nyquist_compliant: true
wave_0_complete: false
created: 2026-06-27
---

# Phase 24 - Validation Strategy

> Per-phase validation contract for Prepare scene data and QRhi bed/plate rendering.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (`PrepareSceneDataTests`, `ViewModelSmokeTests`, `QmlUiAuditTests`) |
| **Config file** | `CMakeLists.txt` |
| **Quick run command** | `build/PrepareSceneDataTests.exe` |
| **Full suite command** | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| **Estimated runtime** | quick: < 10 seconds; full: environment dependent |

---

## Sampling Rate

- **After every task commit:** Run the task-specific static check and the focused Qt test when the binary exists.
- **After every plan wave:** Run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` if code changed and build artifacts are consistent.
- **Before verification:** Canonical full command must be green.
- **Max feedback latency:** 60 seconds for focused static/audit checks.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 24-01-T1 | 01 | 1 | RHI-04, PREP-01 | T-24-01 | Invalid bed dimensions do not generate unbounded buffers | unit | `build/PrepareSceneDataTests.exe` | new | green |
| 24-01-T2 | 01 | 1 | RHI-04, PREP-05 | T-24-02 | Out-of-range active plate becomes empty context | unit | `build/PrepareSceneDataTests.exe` | new | green |
| 24-01-T3 | 01 | 1 | PREP-01 | N/A | Generated bed geometry has deterministic counts | unit/static | `build/PrepareSceneDataTests.exe` | new | green |
| 24-02-T1 | 02 | 2 | PREP-05 | T-24-02 | Active plate object indices come from C++ service truth | unit | `build/ViewModelSmokeTests.exe` | yes | green |
| 24-02-T2 | 02 | 2 | PREP-01, PREP-05 | N/A | QML binds viewmodel state only; no filtering logic | static | `build/QmlUiAuditTests.exe` | yes | green |
| 24-02-T3 | 02 | 2 | RHI-04 | N/A | RhiViewport mirrors state and marks updates only through setters | static/unit | `build/QmlUiAuditTests.exe` | yes | green |
| 24-03-T1 | 03 | 3 | PREP-01, RHI-04 | T-24-01 | QRhi buffer size follows scene geometry counts | static/build | `build/QmlUiAuditTests.exe` | yes | green |
| 24-03-T2 | 03 | 3 | PREP-01 | N/A | Bed/grid/origin render path replaces diagnostic triangle | static/build | `build/QmlUiAuditTests.exe` | yes | green |
| 24-03-T3 | 03 | 3 | PREP-05 | T-24-02 | Active plate switch triggers plate dirty upload path | static/unit | `build/PrepareSceneDataTests.exe`; `build/QmlUiAuditTests.exe` | yes | green |
| 24-04-T1 | 04 | 4 | RHI-04, PREP-01, PREP-05 | N/A | Canonical full verification remains default-safe | full | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | yes | pending |
| 24-04-T2 | 04 | 4 | RHI-04 | N/A | Verification records dirty upload evidence honestly | static/docs | `rg -n "RHI-04|PREP-01|PREP-05|24-VERIFICATION" .planning` | new | pending |

*Status: pending / green / red / flaky*

---

## Wave 0 Requirements

- [x] Add `PrepareSceneDataTests.cpp` and test target before implementing scene data.
- [x] Add QML/static audit checks before wiring Prepare bindings.
- [ ] Preserve Phase 23 default SoftwareViewport startup checks.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| QRhi viewport starts with bed/grid renderer path when explicitly enabled | PREP-01 | Canonical smoke does not enable QRhi or inspect pixels | `OWZX_RHI_RENDERER=1` launch stayed running for 5 seconds and logged `selected=d3d12 attempts=[d3d12:ok]` on 2026-06-27. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies.
- [x] Sampling continuity: no 3 consecutive implementation tasks without automated verify.
- [x] Wave 0 gaps are represented in Plan 24-01.
- [x] No watch-mode flags.
- [x] Feedback latency < 60s for focused checks.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-06-27
