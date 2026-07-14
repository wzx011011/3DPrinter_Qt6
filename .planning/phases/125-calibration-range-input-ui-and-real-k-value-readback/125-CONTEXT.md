# Phase 125: Calibration Range Input UI And Real K-Value Readback - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close CALIB-02 (range input UI replaces hardcoded service ranges) + CALIB-03 (real K-value readback replaces mock). Depends on Phase 124 (the new modes + their hardcoded ranges now exist).

</domain>

<decisions>
## Implementation Decisions

### CALIB-02: range input UI
CalibrationDialog.qml is currently a progress-only dialog. Add range input fields (start/end/step) so the user controls the sweep. The CalibrationServiceMock hardcoded ranges (Phase 124: e.g. Vol_speed 5-30 step 0.5) become defaults that the user can override. Flow: QML inputs → CalibrationViewModel → CalibrationServiceMock → setCalibParams(start,end,step).

### CALIB-03: real K-value readback
Current writeback is mock (`0.04f + item*0.01` at CalibrationServiceMock.cpp:514). Replace with real readback from the sliced G-code where libslic3r provides it. For PA (PressureAdvance), the sliced G-code contains `; M900 K...` / `; pressure_advantage` comments; parse them. For FlowRate/TempTower etc., the result is read from the print itself (manual interpretation upstream). Where libslic3r provides no machine-readable result, document the limitation honestly + tell the user how to read the calibration print manually.

</decisions>

<specifics>
## Code Access Points
- CalibrationDialog.qml (currently progress-only; add range inputs).
- CalibrationServiceMock.cpp (hardcoded ranges → defaults; setCalibParams flow at :444).
- CalibrationViewModel.h (currentKValue/currentNValue — currently display-only).
- SliceService.cpp (G-code output path — parse K-value from sliced output).

## Source-Truth Anchors
- Upstream calibration result parsing: `third_party/OrcaSlicer/src/slic3r/CalibUtils.cpp` + GCode `; M900 K` markers.
- Calib_Params: `calib.hpp:34-47`.

</specifics>

<deferred>
## Deferred Ideas
None — Phase 125 closes WS3 (with Phase 124).

</deferred>
