---
phase: 55
slug: g-code-preview-semantics-and-rendering-stability
status: ready-for-verify
nyquist_compliant: true
wave_0_complete: true
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
| 55-01-T1 | 55-01 | 0 | GCODE-01 | OrcaSlicer-style fixture (12 roles, tags, 2 layers, travels, tool change) backs parser/no-placeholder tests without a live slice | source-audit | grep fixture checks (in task verify) | ✅ green |
| 55-01-T2 | 55-01 | 0 | GCODE-02 | PreviewParserTests target registered + RED scaffold (fixture presence green; 3 RED-by-skip slots) | unit | `ctest --output-on-failure -R PreviewParser` | ✅ green |
| 55-02-T1 | 55-02 | 1 | GCODE-02 | GCV1 wire format gains canonical `int role` (PackedSegment/GcvPackedSegment 76 bytes, lockstepped); render-side role skip; showTravelMoves_=false; toggleRoleVisibility no-repack | build | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | ✅ green |
| 55-02-T2 | 55-02 | 1 | GCODE-02 | 20-role canonical libvgcode parser replaces styleFor; 17 upstream EViewType modes; divergent-role-color guard; PreviewParser RED slots flip GREEN | unit | `ctest --output-on-failure -R PreviewParser` | ✅ green |
| 55-03-T1 | 55-03 | 2 | GCODE-02 | VisibilityFilter.qml (Cx* controls + Theme tokens) + qml.qrc registration | build | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | ✅ green |
| 55-03-T2 | 55-03 | 2 | GCODE-02 | PreviewPage.qml inserts VisibilityFilter + binds GLViewport.roleVisibility (render-side filter fires) | build + audit | `ctest --output-on-failure -R QmlUiAudit` | ✅ green |
| 55-04-T1 | 55-04 | 2 | GCODE-02, GCODE-03 | Role-toggle no-repack (hard QVERIFY2) + legend/global-scope + currentMove/G-code-text atomicity + 17-mode belt-and-suspenders | unit | `ctest --output-on-failure -R ViewModelSmoke` | ✅ green |
| 55-04-T2 | 55-04 | 2 | GCODE-04 | PreviewPage no SoftwareViewport + computePreviewDrawRange role-skip + GcvPackedSegment sizeof 76 audit guards | source-audit | `ctest --output-on-failure -R QmlUiAudit` | ✅ green |
| 55-04-T3 | 55-04 | 2 | GCODE-01, GCODE-05 | No-placeholder live-slice RED (hard QVERIFY2) + sliceFailed/sliceResultCleared/plate-switch reset + reslice/export/page-switch preservation | unit/integration | `ctest --output-on-failure -R E2EWorkflow` | ✅ green |
| 55-05-T1 | 55-05 | 3 | GCODE-04 | Phase-55-tagged D3D11 startup-policy audit (RhiViewport default registration, SoftwareViewport fallback-only behind env gate, PreviewPage binds GLViewport) | source-audit | `ctest --output-on-failure -R QmlUiAudit` | ✅ green |
| 55-05-T2 | 55-05 | 3 | GCODE-04, GCODE-05 | 55-VALIDATION.md frontmatter signed off + Per-Task Verification Map populated + Test Index | doc-audit | `grep -c "nyquist_compliant: true" 55-VALIDATION.md` | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] G-code fixture: commit a small realistic OrcaSlicer-generated `.gcode` file under `tests/fixtures/` with multiple extrusion roles (`;TYPE:`), travel moves, and tagged comments (`;HEIGHT:`, `;WIDTH:`, `;JERK:`, fan/temp/accel) — backs parser/view-mode/role tests without depending on a live slice. **Delivered: tests/fixtures/orca_sample.gcode (Plan 55-01-T1).**
- [x] New parser test file (e.g. `tests/PreviewParserTests.cpp`) registered in CMakeLists for the 20-role parse + 17-mode coverage that `E2EWorkflowTests`/`ViewModelSmokeTests` don't naturally host. **Delivered: tests/PreviewParserTests.cpp + CMakeLists registration (Plan 55-01-T2).**

*Existing QtTest + CTest infrastructure covers all other phase requirements.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Drag stability (camera orbit/pan/zoom, layer slider, move slider) leaves toolpath visibly intact | GCODE-05 | Headless GPU capture unreliable; renderer blanking is a visual runtime behavior | Run `build/OWzxSlicer.exe`, slice a model, switch to Preview, perform each drag interaction, confirm toolpath never disappears. (Carry into Phase 58 UAT.) |
| VisibilityFilter UI matches screenshot + upstream role ordering/defaults | GCODE-02 | Visual layout parity | Run app, open Preview right panel, confirm "Visible Line Types" group shows 20 roles in upstream order with correct default checkboxes and render-side filtering. (Phase 58 UAT.) |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references (fixture + parser test file)
- [x] No watch-mode flags
- [x] Feedback latency < 60s (quick audit)
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** ready-for-verify (Phase 55 planning complete)

---

## Phase 55 Test Index

Canonical commands to run for full phase verification (all confirmed green during Plan 55-05 execution):

- `ctest --output-on-failure -R PreviewParser`  — parser + 20-role + 17-mode + divergent-role-color coverage
- `ctest --output-on-failure -R ViewModelSmoke`  — role-toggle no-repack + legend/global-scope + currentMove atomicity + 17-mode belt-and-suspenders
- `ctest --output-on-failure -R QmlUiAudit`      — D3D11 startup-policy + SoftwareViewport absence + computePreviewDrawRange role-skip + GcvPackedSegment sizeof 76
- `ctest --output-on-failure -R E2EWorkflow`     — no-placeholder live-slice + sliceFailed/sliceResultCleared/plate-switch reset + reslice/export/page-switch preservation
- `ctest --output-on-failure`                    — full suite (PrepareScene + PartPlate + the four above)
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`  — canonical build + all-test gate (the phase-close gate)
