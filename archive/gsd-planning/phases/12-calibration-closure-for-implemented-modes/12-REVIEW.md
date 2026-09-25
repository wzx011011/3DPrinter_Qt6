---
phase: 12
phase_name: Calibration Closure for Implemented Modes
status: reviewed
reviewed: 2026-06-25
scope:
  - src/core/services/CalibrationServiceMock.cpp
  - src/core/services/CalibrationServiceMock.h
  - src/core/viewmodels/CalibrationViewModel.cpp
  - src/core/viewmodels/CalibrationViewModel.h
  - src/qml_gui/BBLTopbar.qml
  - src/qml_gui/BackendContext.cpp
  - src/qml_gui/pages/CalibrationPage.qml
  - tests/ViewModelSmokeTests.cpp
---

# Phase 12 Code Review

## Findings

No open blocking findings remain.

## Fixed During Review

- `src/qml_gui/pages/CalibrationPage.qml`: the unavailable-state background initially used `Qt.rgba(0xf5, 0xa6, 0x23, 0.12)`. QML color components are normalized `0..1`, so that value would clamp incorrectly. It was corrected to `Qt.rgba(0.96, 0.65, 0.14, 0.12)`.

## Review Notes

- Calibration mode routing is stable-id based. QML no longer depends on list index ordering for Flow Dynamics, Flow Rate, or Temp Tower.
- Non-startable calibration modes are rejected in both `CalibrationServiceMock::startCalibration()` and `CalibrationViewModel::startCalibration()`.
- Tests cover both no-`SliceService` fallback and SliceService callback-driven progress/finish/failure paths.
- Existing ViewModel smoke test instability was fixed by aligning test `QSettings` org/app with `main_qml.cpp` and by waiting for asynchronous network probe completion.

## Residual Risk

- Phase 12 does not implement hardware calibration transport or Max Volumetric Speed generation. These are intentionally classified as Blocked/Pending and remain future work.
- The calibration dialog still contains unrelated historical mojibake text outside the Phase 12 behavior path; broader visible UI copy cleanup belongs to Phase 14.
