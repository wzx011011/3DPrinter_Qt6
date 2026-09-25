# Phase 124 Summary: Software-Sliceable Calibration Mode Completion

**Phase:** 124 (WS3, CALIB-01)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
Closed CALIB-01: added the 3 libslic3r tower modes (Vol_speed=7, VFA=8, Retraction=9). Now 6/9 software modes dispatch (PA/FlowRate/TempTower + Vol_speed/VFA/Retraction). SliceService/Print/GCode needed NO changes (transparent passthrough).

## Changes
- CalibrationServiceMock.cpp: upgraded max_volumetric_speed (calibMode 0→7, removed "Pending"), added vfa_tower (calibMode=8), retraction_tune (calibMode=9). Hardware modes unchanged.
- QmlUiAuditTests.cpp: calibrationTowerModesDispatchToLibslic3r slot.
- ViewModelSmokeTests.cpp: updated calibrationUnsupportedModesAreExplicitlyUnavailable (only 2 hardware modes now) + calibrationImplementedModesExposeStableRouting (added 3 tower modes).

## Verification
Canonical build (j6) exit 0; all 5 ctest groups PASS; APP_RUNNING_PID=26948.

## Carry-Forward
- Range input UI + K-value readback → Phase 125.
- Dedicated .drc tower models (geometry tech-debt) → future.
