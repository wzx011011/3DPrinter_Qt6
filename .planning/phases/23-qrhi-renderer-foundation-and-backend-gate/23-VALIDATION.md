---
phase: 23
slug: qrhi-renderer-foundation-and-backend-gate
status: approved
nyquist_compliant: true
wave_0_complete: false
created: 2026-06-27
---

# Phase 23 - Validation Strategy

> Per-phase validation contract for QRhi renderer foundation work.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (`QmlUiAuditTests`) plus optional `owzx-render-bench` executable |
| **Config file** | `CMakeLists.txt` |
| **Quick run command** | `build/QmlUiAuditTests.exe` |
| **Full suite command** | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| **Estimated runtime** | quick: < 10 seconds; full: environment dependent |

---

## Sampling Rate

- **After every task commit:** Run the task-specific static check and `build/QmlUiAuditTests.exe` when the binary exists.
- **After every plan wave:** Run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- **Before `$gsd-verify-work`:** Canonical full command must be green; optional benchmark should be run with `OWZX_RENDER_BENCH=1`.
- **Max feedback latency:** 60 seconds for quick static/audit checks.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 23-01-T1 | 01 | 1 | RHI-01, RHI-02, PERF-05 | T-23-01 | Unknown backend env values fall back safely | static/unit | `build/QmlUiAuditTests.exe` | partial | pending |
| 23-01-T2 | 01 | 1 | RHI-05 | T-23-02 | QML only binds properties; no backend logic | static/build | `build/QmlUiAuditTests.exe` | partial | pending |
| 23-02-T1 | 02 | 2 | RHI-03, RHI-05 | N/A | Renderer shader resources are build-time compiled | build | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | partial | pending |
| 23-02-T2 | 02 | 2 | RHI-01, RHI-02 | T-23-01 | App falls back to stable viewport on QRhi preflight failure | static/build | `build/QmlUiAuditTests.exe` | partial | pending |
| 23-03-T1 | 03 | 3 | RHI-06, PERF-05 | N/A | Benchmark emits structured backend metrics | benchmark | `build/owzx-render-bench.exe --segments 1000 --frames 5 --backend all` | yes | pending |
| 23-03-T2 | 03 | 3 | RHI-01, RHI-03, RHI-05 | T-23-01 | Canonical startup remains unmodified by benchmark env | full | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | yes | pending |

*Status: pending / green / red / flaky*

---

## Wave 0 Requirements

- [ ] Extend `tests/QmlUiAuditTests.cpp` to check `OWZX_RHI_RENDERER`, `RhiViewport`, D3D12/D3D11 policy, and default no-QRhi canonical startup.
- [ ] Add app QRhi source/shader entries to `CMakeLists.txt`.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Visual QRhi host appears when explicitly enabled | RHI-05 | Headless/canonical smoke does not prove visual pixels in the interactive app | Launch with `OWZX_RHI_RENDERER=1` after build and confirm app does not show a blank viewport. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 gaps are represented as first tasks in the plans.
- [x] No watch-mode flags.
- [x] Feedback latency < 60s for quick checks.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-06-27
