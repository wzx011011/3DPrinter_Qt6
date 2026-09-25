# Phase 12 Research: Calibration Closure for Implemented Modes

## Upstream Source Truth

- `third_party/OrcaSlicer/src/slic3r/GUI/BBLTopbar.cpp`
  - Registers `ID_CALIB` and opens `m_calib_menu` from the toolbar calibration button.
- `third_party/OrcaSlicer/src/slic3r/GUI/CalibrationPanel.cpp`
  - Defines user-facing calibration tab names for `Calib_PA_Line`, `Calib_Flow_Rate`, `Calib_Vol_speed_Tower`, `Calib_Temp_Tower`, and `Calib_Retraction_tower`.
- `third_party/OrcaSlicer/src/slic3r/GUI/CalibrationWizard.cpp`
  - Owns PA and Flow Rate wizard flows and `Calib_Params` construction.
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`
  - Routes `Calib_Params` to PA, Temp Tower, Volumetric Speed, Retraction, VFA, Input Shaping, and Cornering generation paths.

## Current Qt6 Implementation

- `src/core/services/CalibrationServiceMock.*`
  - Owns calibration metadata, status, progress, timer fallback, and `SliceService` integration.
  - Currently maps:
    - `flow_dynamics` -> mode `1` (`Calib_PA_Line`)
    - `flow_rate` -> mode `5` (`Calib_Flow_Rate`)
    - `temp_tower` -> mode `6` (`Calib_Temp_Tower`) in code, but no visible metadata item currently exists.
  - Current visible metadata items are Flow Dynamics, Flow Rate, Bed Leveling, Vibration Compensation, and Max Volumetric Speed.
- `src/core/viewmodels/CalibrationViewModel.*`
  - Exposes selected index, start/cancel/reset, step state, history, and parameter state to QML.
  - Does not expose stable calibration ids, implemented/pending status, or startability.
- `src/qml_gui/BBLTopbar.qml`
  - Has a calibration submenu, but the labels are encoding-damaged and item mapping is index-based.
- `src/qml_gui/pages/CalibrationPage.qml` and `src/qml_gui/dialogs/CalibrationDialog.qml`
  - Provide visible calibration center and start dialog.
  - Start action opens a dialog and calls `calibrationVm.startCalibration()`.
- `tests/ViewModelSmokeTests.cpp`
  - Contains an environment-dependent PA G-code generation test.
  - Needs deterministic coverage for mode mapping, mock fallback, and SliceService progress callback behavior.

## Implementation Direction

- Add service/viewmodel APIs for:
  - stable calibration id/name/status access;
  - whether a calibration item has a real slice path;
  - pending/blocked reason for unsupported items;
  - deterministic `calibrationSliceRequested` signal with mode/start/end/step/printNumbers/projectName.
- Add a `temp_tower` calibration item so the three implemented modes are visible and testable.
- Ensure unsupported calibration items cannot silently complete through the mock timer when a user starts them from the implemented-mode path; they should be classified Pending/Blocked.
- Keep QML free of business rules: QML may call `selectItemById()` and `startCalibration()`, but mode classification belongs in C++.
- Keep existing deeper G-code test as evidence, but add fast tests that do not require a real slice export for every mode.

## Verification Targets

- `ViewModelSmokeTests.exe` should cover:
  - implemented mode ids and slice request parameters for PA, Flow Rate, Temp Tower;
  - unimplemented modes report not startable/pending or blocked reason;
  - no-SliceService fallback still completes;
  - SliceService progress/finish/failure callbacks drive state for real dispatched jobs.
- Canonical verification command must pass after implementation:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
