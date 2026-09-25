# Phase 124: Software-Sliceable Calibration Mode Completion - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close CALIB-01: add the libslic3r-supported Calibration tower modes that Qt6 does not yet dispatch. Real slice path today: PA(1)/FlowRate(5)/TempTower(6). Add Vol_speed(7)/VFA(8)/Retraction(9) → 6/9 software modes. Hardware modes (ManualLeveling/BedLeveling/Vibration) stay out of scope.

Note: FlowRateProxy (the 4th candidate in the REQUIREMENTS list) resolves to Calib_Flow_Rate=5, which is ALREADY implemented — so the net-new modes are the 3 tower modes (Vol_speed/VFA/Retraction).

</domain>

<decisions>
## Implementation Decisions (from code research)

### Minimal viable path: add 3 CalibrationType entries + set non-zero calibMode
The dispatch path (`CalibrationServiceMock.cpp:340-378`) is already generic — it checks `calibMode != 0` and forwards to SliceService. SliceService::setCalibParams (`SliceService.cpp:1471-1475`) + the worker injection (`SliceService.cpp:566-579`) is transparent passthrough (`static_cast<CalibMode>(int)`) — no whitelist. So **adding the 3 modes is a CalibrationServiceMock.cpp data change only** (new CalibrationType structs in buildMockData with non-zero calibMode + startable=true + implemented=true). SliceService/Print/GCode need NO changes.

### libslic3r GCode branches already exist for all 3
`GCode.cpp:4617` (Vol_speed: outer_wall_speed per mm), `:4612` (VFA: outer_wall_speed per 5mm), `:4622` (Retraction: retraction_length per layer). They read start/end/step from Calib_Params, which SliceService already fills.

### Known tech-debt: tower geometry uses current-plate model, not upstream's dedicated .drc
Upstream CalibUtils loads dedicated test-tower models (`resources/calib/.../*.drc`/`.stl`) and applies per-mode config overrides (spiral_mode, wall_loops, etc.). Qt6 slices the current-plate geometry via `cloneCurrentPlateModel()`. The GCode parameter injection (temp/speed/retraction sweep) works regardless of geometry, but the resulting tower SHAPE won't be upstream's precision test-tower. This is documented as tech-debt, NOT a Phase 124 blocker — the mode dispatch + GCode generation is the CALIB-01 deliverable. A future "dedicated calibration model loading" enhancement can close the geometry gap.

### Mode → CalibMode mapping
- MaxVolumetricSpeed → Calib_Vol_speed_Tower = 7
- VFA (VolumetricRate) → Calib_VFA_Tower = 8
- RetractionTune → Calib_Retraction_tower = 9
- FlowRateProxy → Calib_Flow_Rate = 5 (ALREADY implemented; NOT re-added)

</decisions>

<specifics>
## Code Access Points
- CalibrationServiceMock.cpp:23-186 (buildMockData — add 3 CalibrationType structs after max_volumetric_speed)
- CalibrationServiceMock.cpp:160-179 (max_volumetric_speed current placeholder — change calibMode 0→7, startable false→true, remove "Pending" reason)
- CalibrationServiceMock.cpp:340-378 (generic dispatch — no change needed)
- SliceService.cpp:1471-1475 (setCalibParams — no change), :566-579 (worker injection — no change)
- CalibrationPage.qml:28-43 (auto-enumerates calibItemCount — new modes appear automatically)

## Source-Truth Anchors
- CalibMode enum: `third_party/OrcaSlicer/src/libslic3r/calib.hpp:16-30`
- GCode branches: `third_party/OrcaSlicer/src/libslic3r/GCode.cpp:4612-4630`
- Calib_Params: `calib.hpp:34-47`

</specifics>

<deferred>
## Deferred Ideas
- Dedicated calibration test-tower model loading (`resources/calib/*.drc` packaging + CalibUtils port) — future enhancement; Phase 124 uses current-plate geometry.
- Per-mode config overrides (spiral_mode/wall_loops) — future, align with CalibUtils.
- Hardware modes (ManualLeveling/BedLeveling/Vibration) — out of scope (printer hardware).

</deferred>
