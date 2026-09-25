---
phase: 12
phase_name: Calibration Closure for Implemented Modes
plan_id: 12-01
title: Close implemented calibration mode workflow
status: complete
wave: 1
type: standard
autonomous: true
requirements_addressed:
  - CAL-01
  - CAL-02
  - CAL-03
  - CAL-04
  - CAL-05
files_modified:
  - src/core/services/CalibrationServiceMock.cpp
  - src/core/services/CalibrationServiceMock.h
  - src/core/viewmodels/CalibrationViewModel.cpp
  - src/core/viewmodels/CalibrationViewModel.h
  - src/qml_gui/BBLTopbar.qml
  - src/qml_gui/BackendContext.cpp
  - src/qml_gui/pages/CalibrationPage.qml
  - tests/ViewModelSmokeTests.cpp
  - .planning/phases/12-calibration-closure-for-implemented-modes/12-SUMMARY.md
  - .planning/phases/12-calibration-closure-for-implemented-modes/12-VERIFICATION.md
  - .planning/phases/12-calibration-closure-for-implemented-modes/12-REVIEW.md
  - .planning/REQUIREMENTS.md
  - .planning/ROADMAP.md
  - .planning/STATE.md
---

# Plan 12-01: Close implemented calibration mode workflow

<objective>
Make PA line, Flow Rate, and Temp Tower calibration visible, startable, and deterministically verified while explicitly classifying unsupported calibration modes as Pending or Blocked. Preserve mock fallback behavior and ensure real slice progress drives calibration status when a real SliceService job is dispatched.
</objective>

<tasks>

1. Add C++ calibration mode classification and request telemetry.
   - Files: `src/core/services/CalibrationServiceMock.h`, `src/core/services/CalibrationServiceMock.cpp`
   - Action: Add stable id lookup, implemented/startable classification, pending/blocked reason accessors, and a `calibrationSliceRequested` signal that emits the selected CalibMode parameters before calling `SliceService::startSlice()`.
   - Action: Add a visible `temp_tower` calibration item mapped to `Calib_Temp_Tower`; classify Bed Leveling and Vibration as hardware blocked, Max Volumetric Speed as pending because Phase 12 does not implement its generation path.
   - Verify: Unit/smoke tests can assert id -> mode/start/end/step without waiting for full G-code export.
   - Acceptance criteria: CAL-02, CAL-04, and CAL-05 have service-level support.

2. Expose minimal viewmodel APIs for QML routing and startability.
   - Files: `src/core/viewmodels/CalibrationViewModel.h`, `src/core/viewmodels/CalibrationViewModel.cpp`
   - Action: Add QML-invokable `selectItemById()`, `calibItemId()`, `calibItemImplemented()`, `calibItemStartable()`, and `calibItemUnavailableReason()`.
   - Action: Make `startCalibration()` refuse non-startable items and emit the normal state/status signals.
   - Verify: ViewModel smoke tests select implemented modes by id and reject unsupported modes deterministically.
   - Acceptance criteria: QML no longer needs to embed calibration business rules.

3. Wire visible calibration entries through stable ids and show unsupported state.
   - Files: `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/pages/CalibrationPage.qml`, `src/qml_gui/dialogs/CalibrationDialog.qml`
   - Action: Replace calibration submenu index routing with `selectItemById()` for `flow_dynamics`, `flow_rate`, and `temp_tower`.
   - Action: Ensure unsupported items display Pending/Blocked reason and cannot start as if completed.
   - Action: Clean only the calibration-related visible mojibake touched by this phase; leave unrelated global UI copy to Phase 14.
   - Verify: Static QML/source scans and QML UI audit continue to pass.
   - Acceptance criteria: CAL-01 and CAL-04 are visible in UI paths.

4. Add deterministic regression coverage.
   - Files: `tests/ViewModelSmokeTests.cpp`
   - Action: Add tests for PA, Flow Rate, and Temp Tower slice request parameter mapping via `calibrationSliceRequested`.
   - Action: Add tests for no-`SliceService` mock fallback and SliceService callback-driven progress/finish/failure behavior.
   - Action: Add tests for unsupported item pending/blocked classification.
   - Verify: `build\ViewModelSmokeTests.exe` passes after build.
   - Acceptance criteria: CAL-02, CAL-03, CAL-04, and CAL-05 have deterministic evidence.

5. Verify and close Phase 12.
   - Files: `.planning/phases/12-calibration-closure-for-implemented-modes/12-SUMMARY.md`, `.planning/phases/12-calibration-closure-for-implemented-modes/12-VERIFICATION.md`, `.planning/phases/12-calibration-closure-for-implemented-modes/12-REVIEW.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`
   - Action: Run targeted tests, run the canonical verification command, write summary/verification/review artifacts, mark CAL requirements complete, and keep unimplemented calibration modes recorded as future work.
   - Verify: canonical verification exits 0 and GSD roadmap analysis identifies Phase 13 as next.
   - Acceptance criteria: Phase 12 has complete artifacts and traceability.

</tasks>

<verification>

- `build\\ViewModelSmokeTests.exe`
- `rg -n "flow_dynamics|flow_rate|temp_tower|calibrationSliceRequested|selectItemById" src/core src/qml_gui tests`
- `git diff --check -- src/core/services/CalibrationServiceMock.cpp src/core/services/CalibrationServiceMock.h src/core/viewmodels/CalibrationViewModel.cpp src/core/viewmodels/CalibrationViewModel.h src/qml_gui/BBLTopbar.qml src/qml_gui/pages/CalibrationPage.qml src/qml_gui/dialogs/CalibrationDialog.qml tests/ViewModelSmokeTests.cpp .planning/phases/12-calibration-closure-for-implemented-modes`
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

</verification>

<success_criteria>

- CAL-01 through CAL-05 are all addressed by the plan.
- PA line, Flow Rate, and Temp Tower are visible and route through stable C++ APIs.
- Unsupported calibration modes are explicit Pending or Blocked work, not silent mock completions.
- Mock fallback and real SliceService progress paths are separately verified.
- Deterministic tests cover mode request parameters without relying on full G-code export for every implemented mode.
- Canonical verification passes and evidence is recorded.

</success_criteria>

## PLANNING COMPLETE
