# Phase 125 Summary: Calibration Range Input UI And Real K-Value Readback

**Phase:** 125 (WS3, CALIB-02 + CALIB-03)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
Closed CALIB-02 (range input UI) + CALIB-03 (real K-value readback). WS3 complete (with Phase 124).

## Changes
- CalibrationServiceMock.h/.cpp: removed mock K-value (0.04f+item*0.01); added parsePressureAdvanceFromGcode (4 firmware variants: M900 K / SET_PRESSURE_ADVANCE / M572 / M233); honest manual-interpretation notes for non-PA modes; range API (setCalibRange + defaults); history hasRealReadback/notes fields.
- CalibrationViewModel.h/.cpp: calibStart/calibEnd/calibStep Q_PROPERTY (defaults from service, setters forward to setCalibRange).
- CalibrationDialog.qml: range input section (3 CxTextField + DoubleValidator, slice modes only).
- QmlUiAuditTests.cpp: calibrationRangeInputAndKValueReadback slot.

## Verification
Canonical build (j6) exit 0; all 5 ctest groups PASS; APP_RUNNING_PID=32080.
