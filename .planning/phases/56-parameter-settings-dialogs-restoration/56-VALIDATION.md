---
phase: 56
slug: parameter-settings-dialogs-restoration
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-07-02
---

# Phase 56 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Visual contract frozen by Phase 50; architecture locked by 56-CONTEXT.md; typed-control matrix in 56-UI-SPEC.md; technical facts in 56-RESEARCH.md.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (QtTest) + CTest (include(CTest) in root CMakeLists.txt) |
| **Config file** | CMakeLists.txt (CTest integration); test targets registered in CMakeLists.txt |
| **Quick run command** | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (canonical: cmake configure + ninja build + smoke test) |
| **Full suite command** | `ctest --test-dir build --output-on-failure` |
| **Estimated runtime** | ~210s for E2EWorkflowTests; ~60s for ViewModelSmokeTests + QmlUiAuditTests + PreviewParserTests |

> Build rule: `scripts/auto_verify_with_vcvars.ps1` is the ONLY full verification command; `build/` is the ONLY build directory. Incremental: `cmake --build build --config Release` only if already configured. See `.claude/rules/build-rules.md`.

---

## Sampling Rate

- **After every task commit:** `cmake --build build --config Release` (incremental) — fast compile feedback
- **After every plan wave:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (full canonical verify)
- **Before `/gsd:verify-work`:** Full suite (`ctest --test-dir build --output-on-failure`) must be green
- **Max feedback latency:** ~210s (E2E suite worst case); incremental build ~30-90s

---

## Per-Requirement → Test Map

| Req ID | Behavior | Test Type | Automated Command | Target File |
|--------|----------|-----------|-------------------|-------------|
| SETTINGS-01 | Dialog opens from sidebar "Setting" button via `settingsRequested(category)`; is a non-modal independent window; correct title/category scope | smoke + QML audit | `ctest --test-dir build -R 'ViewModelSmokeTest|QmlUiAudit'` | ViewModelSmokeTests.cpp + QmlUiAuditTests.cpp |
| SETTINGS-02 | Top tabs + left group-nav populate with screenshot-aligned page/group names per tier (printer/material/process) | unit | `ctest --test-dir build -R ViewModelSmokeTest` | ViewModelSmokeTests.cpp |
| SETTINGS-03 | `ConfigOptionModel` returns correct type/unit/enum/min/max for ALL 7 types (bool, int, float, enum, string, percent, nullable, isVector/multi-value) | unit | `ctest --test-dir build -R ViewModelSmokeTest` | ViewModelSmokeTests.cpp |
| SETTINGS-04 | Per-option dirty detection, value-source/inheritance indicator, builtin read-only gating, validation warning vs blocking error | unit | `ctest --test-dir build -R ViewModelSmokeTest` | ViewModelSmokeTests.cpp |
| SETTINGS-05 | Save (overwrite user preset), Save As (builtin → forced), reset-option, reset-group, reset-all, discard, cancel; UnsavedChangesDialog guard on dirty close/switch | unit | `ctest --test-dir build -R ViewModelSmokeTest` | ViewModelSmokeTests.cpp |
| SETTINGS-06 | Per-dialog search filters option model; basic/advanced toggle uses ConfigOptionMode (4-level); filtered/no-match states | unit | `ctest --test-dir build -R ViewModelSmokeTest` | ViewModelSmokeTests.cpp |
| SETTINGS-07 | Option edit → slice invalidation (reuses Phase-52 `configVm.stateChanged → editorVm.invalidateSliceResultsForAllPlates`); dirty overrides persist via 3MF scoped-config path | integration | `ctest --test-dir build -R E2E` | E2EWorkflowTests.cpp |

---

## Per-Task Verification Map

> Filled with actual Task IDs during/after planning (PLAN.md task IDs). TBD rows are resolved at VALIDATION finalize (end of execute), per Phase-55 pattern.

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| TBD (post-planning) | — | — | SETTINGS-01..07 | — (N/A — local desktop UI, no network/auth surface) | N/A | unit/smoke/integration | see map above | ❌ W0 | ⬜ pending |

---

## Wave 0 Requirements

- [ ] `tests/ViewModelSmokeTests.cpp` — add: `testSettingsDialogOpenFromSidebar()`, `testTabsAndGroupNavPerTier()`, `testConfigOptionModelSevenTypes()`, `testPerOptionDirtyAndValueSource()`, `testReadonlyBuiltinGating()`, `testSaveSaveAsResetOptionResetGroupResetAll()`, `testUnsavedChangesGuardOnDirtyClose()`, `testPerDialogSearchAndFourLevelMode()`, `testNullableAndVectorOptions()`
- [ ] `tests/E2EWorkflowTests.cpp` — add: `testSettingsEditInvalidatesSlice()` (SETTINGS-07: option edit → stalePlateIndices updates / hasStaleSliceResults true), `testDirtyOverridesPersistAcrossProjectSaveRestore()`
- [ ] `tests/QmlUiAuditTests.cpp` — add: `settingsDialogUsesOnlyCxControls()`, `settingsDialogNoRawControls()`, `settingsDialogStringsQsTr()` (copywriting contract)
- [ ] `src/qml_gui/controls/CxSpinBox.qml` — add `property string suffix: ""` for unit-suffix rendering (UI-SPEC flagged; research recommends extending CxSpinBox rather than a new CxUnitSpinBox)
- [ ] No framework install needed — Qt Test already integrated.

*Wave 0 = test scaffolding + the CxSpinBox suffix property; created in the first plan.*

---

## Manual-Only Verifications

> Deferred to Phase 58 (End-to-End Visual and Functional Verification) per the milestone plan. Headless Qt Test cannot reliably assert visual parity.

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Settings dialog visual parity with `shotScreen/打印机参数设置页.png` + `材料参数设置页.png` (density, spacing, control placement, tab/group-nav layout) | SETTINGS-01, SETTINGS-02 | Visual layout parity; headless cannot screenshot-compare | Open each dialog from Prepare sidebar; compare module placement/density to screenshots |
| Typed-control rendering per option type (unit suffix, enum combo, nullable/inherit indicator, validation error row visuals) | SETTINGS-03, SETTINGS-04 | Visual rendering of state | Edit one option of each type; verify unit suffix, dirty marker, inherit indicator, error styling match UI-SPEC |
| Non-modal independence (dialog stays open, Prepare reacts live to edits) | SETTINGS-01 | Cross-window interaction | Open settings, edit an option, confirm Prepare sidebar dirty + slice-stale indicators update without closing dialog |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references (CxSpinBox suffix + test stubs)
- [ ] No watch-mode flags
- [ ] Feedback latency < 210s
- [ ] `nyquist_compliant: true` set in frontmatter (at finalize)

**Approval:** pending
