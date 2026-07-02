---
phase: 55
slug: g-code-preview-semantics-and-rendering-stability
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-07-02
---

# Phase 55 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (QtTest) + CTest (already integrated in root CMakeLists.txt) |
| **Config file** | `CMakeLists.txt` (root, `include(CTest)` + `add_test`); tests in `tests/` |
| **Quick run command** | `ctest --output-on-failure -R QmlUiAudit` (source-audit tests, <5s) |
| **Full suite command** | `ctest --output-on-failure` |
| **Canonical build+test** | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| **Estimated runtime** | ~30-60s quick (audit); full suite via canonical script (compile-bound) |

---

## Sampling Rate

- **After every task commit:** Run `ctest --output-on-failure -R QmlUiAudit` (or the targeted `-R` filter for the touched test file)
- **After every plan wave:** Run `ctest --output-on-failure` (full suite)
- **Before `/gsd:verify-work`:** Full canonical verification (`auto_verify_with_vcvars.ps1`) must pass
- **Max feedback latency:** ~60s for quick audit; canonical build is the gate before phase close

---

## Requirement-Level Verification Map

| Requirement | Behavior | Test Type | Automated Command | Extends | Wave 0 Gap? |
|-------------|----------|-----------|-------------------|---------|-------------|
| GCODE-01 | After live slice, `gcodePreviewData` non-empty GCV1 with real segments (no placeholder) | unit/integration | `ctest --output-on-failure -R E2EWorkflow` | `E2EWorkflowTests.cpp` | No |
| GCODE-01 | `sliceFailed` / `sliceResultCleared` triggers full `resetPreviewState` | unit | `ctest --output-on-failure -R E2EWorkflow` | existing | No |
| GCODE-01 | `resultChanged` (plate switch to valid result) rebuilds Preview | unit | `ctest --output-on-failure -R E2EWorkflow` | existing | No |
| GCODE-02 | 20 `EGCodeExtrusionRole` values parsed from `;TYPE:` comments | unit | `ctest --output-on-failure -R PreviewParser` | new test | Yes — Wave 0 |
| GCODE-02 | Role visibility toggle does NOT repack `gcodePreviewData` (update() only) | unit | `ctest --output-on-failure -R ViewModelSmoke` | `ViewModelSmokeTests.cpp` | No |
| GCODE-02 | 17 upstream `EViewType` modes present and correctly ordered/labeled | unit | `ctest --output-on-failure -R ViewModelSmoke` | `ViewModelSmokeTests.cpp` | No |
| GCODE-03 | Legend min/max unchanged on slider drag (global scope) | unit | `ctest --output-on-failure -R ViewModelSmoke` | `ViewModelSmokeTests.cpp` | No |
| GCODE-03 | G-code text window + `currentGcodeLine` update atomically with `setCurrentMove` | unit | `ctest --output-on-failure -R ViewModelSmoke` | `ViewModelSmokeTests.cpp` | No |
| GCODE-03 | Layer/move range update draw range + current-line together | unit | `ctest --output-on-failure -R ViewModelSmoke` | `ViewModelSmokeTests.cpp` | No |
| GCODE-04 | `PreviewPage.qml` never references `SoftwareViewport` | source-audit | `ctest --output-on-failure -R QmlUiAudit` | `QmlUiAuditTests.cpp` | No |
| GCODE-04 | `main_qml.cpp` registers `RhiViewport` as default `GLViewport` | source-audit | `ctest --output-on-failure -R QmlUiAudit` | existing | No |
| GCODE-05 | `gcodePreviewData` survives layer/move/camera/toggle interaction setters | unit | `ctest --output-on-failure -R E2EWorkflow` | existing invariant | No |
| GCODE-05 | Reslice clears and rebuilds `gcodePreviewData` (bytes change, counts recompute) | unit | `ctest --output-on-failure -R E2EWorkflow` | `E2EWorkflowTests.cpp` | No |
| GCODE-05 | Export while Preview visible leaves `gcodePreviewData` intact | unit | `ctest --output-on-failure -R E2EWorkflow` | `E2EWorkflowTests.cpp` | No |
| GCODE-05 | Page switch (Prepare↔Preview) preserves `gcodePreviewData` | unit | `ctest --output-on-failure -R E2EWorkflow` | `E2EWorkflowTests.cpp` | No |

---

## Per-Task Verification Map

> Task IDs assigned by the planner. Each task's `<automated>` verify must map to a row above.

| Task ID | Plan | Wave | Requirement | Secure Behavior | Test Type | Automated Command | Status |
|---------|------|------|-------------|-----------------|-----------|-------------------|--------|
| TBD | TBD | TBD | TBD | TBD | TBD | TBD | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] G-code fixture: commit a small realistic OrcaSlicer-generated `.gcode` file under `tests/fixtures/` with multiple extrusion roles (`;TYPE:`), travel moves, and tagged comments (`;HEIGHT:`, `;WIDTH:`, `;JERK:`, fan/temp/accel) — backs parser/view-mode/role tests without depending on a live slice.
- [ ] New parser test file (e.g. `tests/PreviewParserTests.cpp`) registered in CMakeLists for the 20-role parse + 17-mode coverage that `E2EWorkflowTests`/`ViewModelSmokeTests` don't naturally host.

*Existing QtTest + CTest infrastructure covers all other phase requirements.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Drag stability (camera orbit/pan/zoom, layer slider, move slider) leaves toolpath visibly intact | GCODE-05 | Headless GPU capture unreliable; renderer blanking is a visual runtime behavior | Run `build/OWzxSlicer.exe`, slice a model, switch to Preview, perform each drag interaction, confirm toolpath never disappears. (Carry into Phase 58 UAT.) |
| VisibilityFilter UI matches screenshot + upstream role ordering/defaults | GCODE-02 | Visual layout parity | Run app, open Preview right panel, confirm "Visible Line Types" group shows 20 roles in upstream order with correct default checkboxes and render-side filtering. (Phase 58 UAT.) |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references (fixture + parser test file)
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s (quick audit)
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
