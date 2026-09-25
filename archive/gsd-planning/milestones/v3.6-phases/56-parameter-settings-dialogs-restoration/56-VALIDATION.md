---
phase: 56
slug: parameter-settings-dialogs-restoration
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-07-02
finalized: 2026-07-03
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

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 56-01-T1 | 56-01 | 1 | SETTINGS-03,06 | N/A (local UI) | N/A | unit | `ctest -R ViewModelSmokeTest` | ✅ | ✅ passed |
| 56-01-T2 | 56-01 | 1 | SETTINGS-03 | N/A | N/A | build | `cmake --build build --config Release` | ✅ | ✅ passed |
| 56-01-T3 | 56-01 | 1 | SETTINGS-01..07 | N/A | N/A | unit | `ctest -R 'ViewModelSmokeTest\|E2E\|QmlUiAudit'` | ✅ | ✅ passed (scaffolds placed, later flipped GREEN) |
| 56-02-T1 | 56-02 | 2 | SETTINGS-02,06 | N/A | N/A | unit | `ctest -R ViewModelSmokeTest` | ✅ | ✅ passed |
| 56-02-T2 | 56-02 | 2 | SETTINGS-01,02,04,05,06 | N/A | N/A | unit | `ctest -R ViewModelSmokeTest` | ✅ | ✅ passed (forwardSettingsRequest spy test GREEN) |
| 56-03-T1 | 56-03 | 3 | SETTINGS-03 | N/A | N/A | build+audit | `cmake --build build --config Release` | ✅ | ✅ passed |
| 56-03-T2 | 56-03 | 3 | SETTINGS-01,02,04,05,06 | N/A | N/A | build+audit | `cmake --build build --config Release` | ✅ | ✅ passed |
| 56-03-T3 | 56-03 | 3 | SETTINGS-01 | N/A | N/A | unit | `ctest -R 'QmlUiAudit\|ViewModelSmokeTest'` | ✅ | ✅ passed (4 QmlUiAudit + 3 VMSmoke scaffolds GREEN) |
| 56-04-T1 | 56-04 | 4 | SETTINGS-07 | N/A | N/A | integration | `ctest -R E2E` | ✅ | ✅ passed (testSettingsEditInvalidatesSlice + testDirtyOverridesPersist) |
| 56-04-T2 | 56-04 | 4 | SETTINGS-03,04,05,06 | N/A | N/A | unit | `ctest -R ViewModelSmokeTest` | ✅ | ✅ passed (covered in 56-03; VMSmoke 85/0) |
| 56-04-T3 | 56-04 | 4 | (VALIDATION finalize) | N/A | N/A | doc-check | `python -c "...assert nyquist_compliant: true..."` | ✅ | ✅ passed |

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

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references (CxSpinBox suffix + test stubs)
- [x] No watch-mode flags
- [x] Feedback latency < 210s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved (automated) — manual visual parity items deferred to Phase 58 per the Manual-Only table.

**Final test results (2026-07-03):**
- ViewModelSmokeTests: **85 passed, 0 failed, 1 skipped** (pre-existing THUMB-03 unrelated skip)
- QmlUiAuditTests: **36 passed, 0 failed**
- E2EWorkflowTests (new SETTINGS-07 tests): **testSettingsEditInvalidatesSlice + testDirtyOverridesPersistAcrossProjectSaveRestore PASS**
- Incremental build: clean (0 errors). Canonical `auto_verify_with_vcvars.ps1` smoke step reported a one-off flaky Qt-environment crash (Qt version-mismatch warning, "crash at any arbitrary point"); 5/5 direct reruns of ViewModelSmokeTests.exe pass stable (85/0). The flake is environmental, not a Phase 56 code defect.
