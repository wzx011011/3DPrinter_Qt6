---
phase: 25
slug: prepare-model-mesh-rendering-and-camera-interaction
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-06-27
---

# Phase 25 - Validation Strategy

> Per-phase validation contract for QRhi Prepare model mesh rendering, camera interaction, and selection/hover feedback.

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
- **After every plan wave:** Run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` after code changes compile.
- **Before verification:** Canonical full command must pass.
- **Max feedback latency:** 60 seconds for focused static/audit checks.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 25-01-T1 | 01 | 1 | PREP-02, PREP-04 | T-25-01 | Malformed mesh bytes do not create unsafe model batches | unit | `build/PrepareSceneDataTests.exe` | yes | green |
| 25-01-T2 | 01 | 1 | PREP-02, PREP-04 | T-25-02 | Source-object mapping is explicit and not inferred from render IDs | unit/static | `build/PrepareSceneDataTests.exe`; `build/QmlUiAuditTests.exe` | yes | green |
| 25-01-T3 | 01 | 1 | PREP-02, PREP-07 | T-25-03 | Active plate filtering excludes inactive source objects | unit | `build/PrepareSceneDataTests.exe` | yes | green |
| 25-02-T1 | 02 | 2 | PREP-02 | T-25-01 | QRhi model buffers are allocated from validated scene data | static/build | `build/QmlUiAuditTests.exe` | yes | green |
| 25-02-T2 | 02 | 2 | PREP-03 | T-25-04 | Camera movement updates uniforms and leaves full model buffers resident | static/build | `build/QmlUiAuditTests.exe` | yes | green |
| 25-02-T3 | 02 | 2 | PREP-02, PREP-07 | N/A | Rendered models use deterministic colors and source-truth mapping comments | static/build | `build/QmlUiAuditTests.exe`; `rg -n "GLCanvas3D|Camera|PartPlate|Selection" src/qml_gui/Renderer` | yes | green |
| 25-03-T1 | 03 | 3 | PREP-04 | T-25-02 | `EditorViewModel` owns selected source object state | unit | `build/ViewModelSmokeTests.exe` | yes | green |
| 25-03-T2 | 03 | 3 | PREP-04 | T-25-05 | QML forwards C++ picked-source signal without filtering or parsing | static | `build/QmlUiAuditTests.exe` | yes | green |
| 25-03-T3 | 03 | 3 | PREP-03, PREP-04 | T-25-04 | Hover/selection feedback updates lightweight state only | static/build | `build/QmlUiAuditTests.exe` | yes | green |
| 25-04-T1 | 04 | 4 | PREP-02, PREP-03, PREP-04, PREP-07 | N/A | Canonical full verification remains default-safe | full | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | yes | green |
| 25-04-T2 | 04 | 4 | PREP-02, PREP-03, PREP-04, PREP-07 | N/A | Verification and requirement traceability record QRhi behavior honestly | docs/static | `rg -n "PREP-02|PREP-03|PREP-04|PREP-07|25-VERIFICATION" .planning` | yes | green |

*Status: pending / green / red / flaky*

---

## Wave 0 Requirements

- [x] Add packed model parser tests before renderer model buffer implementation.
- [x] Add QML/static audit checks before adding new viewport bindings.
- [x] Preserve Phase 23 default SoftwareViewport startup checks.
- [x] Preserve Phase 24 active-plate bed/plate dirty-flag tests.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| QRhi model viewport renders loaded model geometry when explicitly enabled | PREP-02 | Canonical smoke does not inspect live pixels | Launch with `OWZX_RHI_RENDERER=1`, load a small model, confirm model is visible on active plate. |
| Camera orbit/pan/zoom/fit remains responsive under QRhi | PREP-03 | Requires interactive mouse input | With `OWZX_RHI_RENDERER=1`, exercise left drag, middle drag, wheel, and toolbar fit action. |
| Click selection and hover feedback match object list selection | PREP-04 | Requires interactive picking | With `OWZX_RHI_RENDERER=1`, hover and click model objects and compare object list selection. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or manual verification rationale.
- [x] Sampling continuity: no 3 consecutive implementation tasks without automated verify.
- [x] Wave 0 gaps are represented in Plan 25-01 and Plan 25-03.
- [x] No watch-mode flags.
- [x] Feedback latency < 60s for focused checks.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-06-27
