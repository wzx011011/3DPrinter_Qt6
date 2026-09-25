---
phase: 12
phase_name: Calibration Closure for Implemented Modes
plan_id: 12-01
status: complete
completed: 2026-06-25
requirements_completed:
  - CAL-01
  - CAL-02
  - CAL-03
  - CAL-04
  - CAL-05
key_files:
  modified:
    - src/core/services/CalibrationServiceMock.cpp
    - src/core/services/CalibrationServiceMock.h
    - src/core/viewmodels/CalibrationViewModel.cpp
    - src/core/viewmodels/CalibrationViewModel.h
    - src/qml_gui/BBLTopbar.qml
    - src/qml_gui/BackendContext.cpp
    - src/qml_gui/pages/CalibrationPage.qml
    - tests/ViewModelSmokeTests.cpp
---

# Phase 12 Summary: Calibration Closure for Implemented Modes

## What Changed

- Added stable calibration capability metadata in `CalibrationServiceMock`:
  - `flow_dynamics` -> CalibMode 1, range `0.0..0.1`, step `0.002`
  - `flow_rate` -> CalibMode 5, range `0.90..1.10`, step `0.01`
  - `temp_tower` -> CalibMode 6, range `190..240`, step `5`
- Added `calibrationSliceRequested(...)` telemetry before dispatching `SliceService::startSlice()` so tests can assert deterministic slice request parameters without waiting for full G-code export.
- Added ViewModel routing APIs for QML:
  - `selectItemById()`
  - `calibItemId()`
  - `calibItemImplemented()`
  - `calibItemStartable()`
  - `calibItemUnavailableReason()`
- Updated `BBLTopbar.qml` so implemented calibration menu entries route by stable ids instead of list indexes.
- Updated `CalibrationPage.qml` so unsupported calibration modes show an explicit Pending/Blocked reason and cannot open the start dialog.
- Kept unsupported modes explicit:
  - Bed Leveling: blocked by live printer hardware calibration support.
  - Vibration Compensation: blocked by live printer resonance measurement support.
  - Max Volumetric Speed: pending outside Phase 12.
- Stabilized existing ViewModel smoke tests that Phase 12 now runs explicitly:
  - Test `QSettings` org/app now match the real app (`OWzx` / `OWzxSlicer`).
  - Monitor network refresh test waits for the asynchronous `networkChanged` signal.
  - Sidebar persistence setters now call `QSettings::sync()` after writes.

## Verification

- Phase 12 targeted calibration tests passed.
- Full `ViewModelSmokeTests.exe` passed: 31 passed, 0 failed.
- QML UI audit passed.
- Canonical verification passed:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - Release build completed.
  - QML UI audit tests passed.
  - E2E pipeline tests passed.

## Remaining Work

- Phase 13: deterministic verification for SSDP/MQTT/FTP/camera/AppSettings/viewport hybrid integration.
- Phase 14: visible placeholder triage for remaining top-level no-op or placeholder surfaces.
- Future milestone: full calibration mode coverage beyond PA, Flow Rate, and Temp Tower.
