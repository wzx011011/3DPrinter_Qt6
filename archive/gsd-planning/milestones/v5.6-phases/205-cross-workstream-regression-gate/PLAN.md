# Phase 205: Cross-Workstream Regression Gate and Milestone Audit

**Milestone:** v5.6
**Workstream:** GATE
**Requirement:** GATE-01
**Status:** Code-complete, pending canonical build verification
**Dependencies:** Phases 193-204 complete (Wave C final gate)

## Goal

Close the v5.6 milestone. Add the consolidated
`v56CrossWorkstreamRegressionLocked` source-audit slot to
`tests/QmlUiAuditTests.cpp` and a paired
`v56CrossWorkstreamViewModelsCallable` smoke slot to
`tests/ViewModelSmokeTests.cpp`, then produce the
`v5.6-MILESTONE-AUDIT.md` that honestly records the milestone state.

This is the standard cross-workstream gate pattern established by v4.5 Phase
116 and continued through v5.4 Phase 187. The v5.6 gate adds the new v5.6
anchors (UI/FEAT/DLG/RHI/I18N) and re-asserts the v5.4/v5.0/v4.6 carry-forward
anchors in one place so a future regression that drops any v5.6 deliverable
fails a named `GATE-01/<requirement-id>:` assertion.

## Scope

### 1. `v56CrossWorkstreamRegressionLocked()` in `tests/QmlUiAuditTests.cpp`

Source-audit slot (QFile + QT_TESTCASE_SOURCEDIR + QString::contains +
QFile::exists + QVERIFY2 with `GATE-01/`-prefixed messages). Runs in the
regression ctest. Each anchor names the requirement it locks:

- **UI-01 (Phase 194)**: CxBadge.qml / CxNumericEdit.qml / CxStepButton.qml /
  CxBusyIndicator.qml exist under `controls/`; OptionRow.qml / MoveSlider.qml /
  PreviewLayerRail.qml contain NO inline `component Badge:` /
  `component NumericEdit:` / `component MoveStepButton:` /
  `component RailButton:` and instead consume the extracted Cx controls.
- **UI-02 (Phase 195)**: `dialogs/KBShortcutsDialog.qml` exists; `main.qml`
  instantiates `KBShortcutsDialog` (not an inline Dialog).
- **FEAT-01 (Phase 196)**: `EditorViewModel.h` keeps the `embossRunning()`
  getter (Emboss spinner); `SliceService.h` keeps the `sliceState`
  Q_PROPERTY + `sliceStateChanged` signal (SliceProgress Cancelled/Error banner).
- **FEAT-02 (Phase 197)**: the 4 bundled tower models exist under
  `assets/calib/`; `CalibrationServiceMock.cpp` keeps `towerModelQrcPathForMode`
  + `extractQrcToTempFile` + `QTemporaryFile`, and contains no `.drc` residue.
- **FEAT-03 (Phase 198)**: `EditorViewModel.h` keeps the `selectedVolumeIndex`
  Q_PROPERTY + getter (ObjectList tree deepening).
- **DLG-01 (Phase 199)**: `PresetServiceMock.h` keeps `vendors()` +
  `printerModelsForVendor()` + `materialsForVendor()` + `bedTypes*()`.
- **DLG-03 (Phase 201)**: `AmsMaterialsViewModel.h` exists with its `slotCount`
  Q_PROPERTY.
- **DLG-04 (Phase 202)**: `PluginService.h` exists with `installPlugin` +
  `uninstallPlugin` + `QSettings` reference.
- **I18N-01 (Phase 204)**: one canonical finished translation per locale
  (`Einstellungen`/de, `Paramètres`/fr, `設定`/ja, `설정`/ko).
- **RHI-01 (Phase 203)**: `RhiBackendSelector.cpp` keeps `OWZX_RHI_RENDERER`
  + the `D3D11-first` rationale comment (D3D12 stays opt-in, default stays
  D3D11).
- **Re-assertion**: v5.4 (`QPointer<RhiViewport>` + `m_selectedSourceIndices`),
  v5.0 (`Slic3r::Emboss::text2shapes`), v4.6 (`calibMode = 7/9`).

### 2. `v56CrossWorkstreamViewModelsCallable()` in `tests/ViewModelSmokeTests.cpp`

Smoke slot proving the compiled symbols are real (not just header text).
Constructs the objects and calls the getters:

- FEAT-01: `editor.embossRunning()` == false; `slice.sliceState()` == Idle.
- FEAT-03: `editor.selectedVolumeIndex()` == -1.
- DLG-01: `preset.vendors()` non-empty; `printerModelsForVendor` /
  `materialsForVendor` / `bedTypesForPrinterModel` callable.
- DLG-03: `AmsMaterialsViewModel` constructs; `slotCount` / `slotNames` /
  `materialTypes` callable.
- DLG-04: `PluginService` constructs; `pluginCount` > 0; `pluginNames` aligns;
  `pluginAt(0)` returns a row with `name` + `isEnabled`.

New includes added: `core/services/PluginService.h`,
`core/viewmodels/AmsMaterialsViewModel.h`.

### 3. `.planning/milestones/v5.6-MILESTONE-AUDIT.md`

Honest milestone record. Status `pending_verification`: code and docs are in
place, but the canonical build + ctest could NOT be run because the build
environment is broken (vcvars64.bat VsDevCmd extension init failure; cl.exe not
on PATH). The audit does NOT claim `clean` or `tech_debt` — it records
`pending_verification` and names the unblock step (fix the VS environment, then
run `scripts/auto_verify_with_vcvars.ps1`).

## Out of Scope

- Any new product behavior.
- Any non-test source change (the gate is purely test + documentation).
- Re-asserting v4.5/v4.7/v4.8/v5.1/v5.2/v5.3 specifically — those anchors are
  covered transitively by their own milestone slots; v5.6 deliberately carries
  the same reduced set as v5.4 to avoid unbounded growth.
- Fixing the VS build environment (that is a user follow-up, not a code change).

## Verification

This phase's verification is the v5.6 Verification Gate from
`v5.6-ROADMAP.md`:

- [ ] **Canonical build exits 0** — `powershell -ExecutionPolicy Bypass -File
      scripts/auto_verify_with_vcvars.ps1`.
- [ ] ctest: QmlUiAuditTests (count +1), ViewModelSmokeTests (count +1),
      PartPlateTests, PrepareSceneDataTests, PreviewParserTests all PASS.
- [ ] E2E workflow PASS.
- [ ] `OWzxSlicer.exe` launch liveness (PID reported).
- [ ] Regression slots intact.

**Current status: BLOCKED.** The canonical build could not be run in this
session because the build environment is broken:

- `vcvars64.bat` (the VS dev-shell bootstrap that `auto_verify_with_vcvars.ps1`
  invokes) fails during VsDevCmd extension initialization.
- `cl.exe` is not injected onto PATH, so the configure/compile step cannot
  start.
- This is an environment/toolchain problem on the host machine, not a product
  regression.

**Unblock (user follow-up):**

1. Repair the Visual Studio install (or the VsDevCmd / VS extension that is
   failing init). A `devenv /setup` or a VS Installer repair is the usual
   remedy; the failing extension shows in the vcvars64 output.
2. Re-run `powershell -ExecutionPolicy Bypass -File
   scripts/auto_verify_with_vcvars.ps1`.
3. If it exits 0 and ctest is green, flip the audit status from
   `pending_verification` to `clean` and close v5.6. If a real test fails,
   fix it and re-run; if a deferred item surfaces, record it under `tech_debt`.

The new test slots were written to match the exact APIs and source strings
verified present by this phase (anchor strings were grep-confirmed before the
slots were written), so the expectation is that the slots will pass once the
build environment is restored — but that expectation is NOT verified.

## Risk Notes

- The `v56CrossWorkstreamRegressionLocked` and
  `v56CrossWorkstreamViewModelsCallable` slots are NOT compiled/run yet. Every
  anchor string and API signature was verified present by direct file read +
  grep during this phase, but a compile error (typo, include path, or an API
  that differs at the `.cpp` from the `.h`) can only be caught by the canonical
  build. The audit records this honestly as `pending_verification`.
- Adding new private slots to a single-file Qt test triggers the AUTOMOC stale
  timestamp caveat (documented at `tests/ViewModelSmokeTests.cpp:287`); the
  canonical verify script re-runs cmake configure, but an incremental build
  needs `build/ViewModelSmokeTests_autogen/timestamp` deleted to pick up the
  new slot.
- The RHI-01 anchor re-uses `d3d12StaysOptInBehindEnvFlag`'s contract; if that
  slot is ever removed, the v5.6 gate still carries the order contract under
  the `GATE-01/RHI-01` message prefix.

## Files

- `tests/QmlUiAuditTests.cpp` — `v56CrossWorkstreamRegressionLocked`
  declaration + implementation.
- `tests/ViewModelSmokeTests.cpp` — `v56CrossWorkstreamViewModelsCallable`
  declaration + implementation; two new includes.
- `.planning/milestones/v5.6-MILESTONE-AUDIT.md` — new milestone audit (this
  phase's primary documentation deliverable).
- `.planning/milestones/v5.6-phases/205-cross-workstream-regression-gate/PLAN.md`
  — this file.
