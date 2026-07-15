# Phase 118: custom_gcode_per_print_z Writeback And Re-Slice Loop - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close TICK-02 + TICK-03: wire PreviewViewModel tick CRUD (add/remove/edit) into libslic3r's per-plate custom-gcode store and trigger a re-slice so the resulting G-code actually contains the user's pause/color-change/filament-change/custom-gcode markers. Phase 117 surfaced the UI; Phase 118 closes the loop (write-back + re-slice).

</domain>

<decisions>
## Implementation Decisions (from code research)

### Critical correction: set_custom_gcode_per_print_z does NOT exist
OrcaSlicer/BBS deprecated `Model::set_custom_gcode_per_print_z`. The real write path is direct field assignment on the `Slic3r::Model` public members (`Model.hpp:1559-1570`):
```cpp
model->curr_plate_index = plateIndex;
model->plates_custom_gcodes[plateIndex] = customGcodeInfo;  // CustomGCode::Info
```
`cloneCurrentPlateModel()` copies `plates_custom_gcodes` (Model.cpp:82-83), so the re-slice via `startSlice()` (which calls clone at SliceService.cpp:352) automatically sees the written custom codes.

### Type enum MUST be explicitly mapped (NOT static_cast)
The two enums have DIFFERENT numeric order:
| TickType (project) | val | CustomGCode::Type (upstream) | val |
|---|---|---|---|
| PausePrint | 0 | PausePrint | 1 |
| CustomGcode | 1 | Custom | 4 |
| Template | 2 | Template | 3 |
| ToolChange | 3 | ToolChange | 2 |
| ColorChange | 4 | ColorChange | 0 |

### layer → print_z via existing layerZAt(layer)
`tick` is a layer index; `CustomGCode::Item.print_z` is a Z height (mm). Convert via `PreviewViewModel::layerZAt(layer)` (PreviewViewModel.cpp:1444-1449), which reads `m_layerZs[layer]`. Guard against out-of-range (returns 0 → would misplace tick at first layer).

### Dependency gap: PreviewViewModel needs projectService_
PreviewViewModel currently only receives `sliceService_` (PreviewViewModel.h:97, BackendContext.cpp:79). To write `rawModel()->plates_custom_gcodes`, add `ProjectServiceMock*` to the constructor (mirroring EditorViewModel's injection at BackendContext.cpp:73).

### Re-slice entry
`sliceService_->startSlice(projectName)` is idempotent re-slice (SliceService.cpp:325-352 clears old result, re-clones model with written custom codes, re-applies). Guard with `sliceService_->slicing()` check.

</decisions>

<specifics>
## Source-Truth Anchors
- Upstream custom-gcode store: `third_party/OrcaSlicer/src/libslic3r/Model.hpp:1559-1570` (curr_plate_index + plates_custom_gcodes).
- CustomGCode::Item/Info/Type: `third_party/OrcaSlicer/src/libslic3r/CustomGCode.hpp:14-111`.
- check_mode_for_custom_gcode_per_print_z: `CustomGCode.hpp:121` (derives Info::mode from items — call after assembling).
- Re-slice path: `SliceService.cpp:325-352` (cloneCurrentPlateModel copies plates_custom_gcodes; Print::apply reads them).

## Code Access Points
- PreviewViewModel tick CRUD: `PreviewViewModel.cpp:1778-1873` (mutate tickMarks_, emit tickMarksChanged — ADD write-back + re-slice here).
- PreviewViewModel ctor: `PreviewViewModel.cpp:346-347` (add projectService_ param).
- PreviewViewModel signals: `PreviewViewModel.h:233-235` (may add a tickEditTriggeredReslice signal if needed for UI feedback).
- BackendContext assembly: `BackendContext.cpp:79` (pass projectService_ to PreviewViewModel ctor).
- SliceService::startSlice: `SliceService.h:189`, `SliceService.cpp:325`.
- ProjectServiceMock::rawModel: `ProjectServiceMock.h:113-114`.

</specifics>

<deferred>
## Deferred Ideas
None — Phase 119 handles 5-type end-to-end round-trip + drag relocation; Phase 118 closes the write-back + re-slice loop for all types that tick CRUD already supports (Pause/CustomGcode/ToolChange via existing methods; ColorChange/Template data already flows through tickMarks_).

</deferred>
