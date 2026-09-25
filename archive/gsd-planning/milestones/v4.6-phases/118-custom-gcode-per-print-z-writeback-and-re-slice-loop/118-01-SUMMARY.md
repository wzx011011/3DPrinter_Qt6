# Phase 118 Summary: custom_gcode_per_print_z Writeback And Re-Slice Loop

**Phase:** 118 — custom_gcode_per_print_z Writeback And Re-Slice Loop (WS1, TICK-02 + TICK-03)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped

Closed TICK-02 + TICK-03: PreviewViewModel tick CRUD (add/remove/edit) now writes back to libslic3r's per-plate custom-gode store and triggers a re-slice, so the resulting G-code contains the user's pause/color-change/filament-change/custom-gcode markers. Phase 117 surfaced the UI; Phase 118 closes the loop.

## Key Correction: set_custom_gcode_per_print_z does NOT exist

Research found OrcaSlicer/BBS deprecated `Model::set_custom_gcode_per_print_z`. The real write path is direct field assignment on `Slic3r::Model` public members (`Model.hpp:1559-1570`):
```cpp
model->curr_plate_index = plateIndex;
model->plates_custom_gcodes[plateIndex] = customGcodeInfo;
```
`cloneCurrentPlateModel()` copies `plates_custom_gcodes` (Model.cpp:82-83), so re-slice via `startSlice()` (SliceService.cpp:352) sees the written codes. The PLAN/context captured this; the implementation uses direct assignment, NOT the nonexistent setter.

## Changes

| File | Change |
|---|---|
| `src/core/viewmodels/PreviewViewModel.h` | Constructor now takes `ProjectServiceMock*` (mirrors EditorViewModel injection). Added `projectService_` member + `writeTicksToModel()` decl. |
| `src/core/viewmodels/PreviewViewModel.cpp` | Added pure helper `convertTicksToCustomGcodeInfo()` (anonymous namespace, HAS_LIBSLIC3R guarded, EXPLICIT TickType→CustomGCode::Type switch — NOT static_cast, since the enum orders differ). `writeTicksToModel()` writes plates_custom_gcodes + curr_plate_index, calls check_mode_for_custom_gcode_per_print_z, triggers startSlice. All 6 CRUD methods wired (add/remove/edit/clear). Non-lib stub emits tickMarksChanged only. |
| `src/qml_gui/BackendContext.cpp` | Assembly passes projectService_ to PreviewViewModel ctor. |
| `tests/QmlUiAuditTests.cpp` | Added `customGcodeWritebackAndResliceWired` source-audit slot. |
| `tests/E2EWorkflowTests.cpp`, `tests/PreviewParserTests.cpp`, `tests/ViewModelSmokeTests.cpp` | Updated all PreviewViewModel constructions to the new 2-arg signature (`&project, &slice`). |

## Key Decisions

1. **Explicit enum switch map** (PausePrint:0→1, CustomGcode:1→4, Template:2→3, ToolChange:3→2, ColorChange:4→0). Verified zero `static_cast<Slic3r::CustomGCode::Type>` — the orders differ and static_cast would silently corrupt.
2. **Pure helper extracted** (`convertTicksToCustomGcodeInfo`) for unit-testability without a Model/SliceService — mirrors the v4.5/v4.6 unit-testable-boundary pattern.
3. **layer→print_z** via existing `layerZAt(layer)`, with out-of-range + invalid-print_z guards (skips ticks that would collapse to layer 0).
4. **Re-slice is idempotent**: startSlice clears old result, re-clones model (with written codes), re-applies. Guarded by `slicing()` check (skip if a slice is in progress; tick edit persists for the next slice).
5. **removeTick/editTick call write-back only on the success branch** — a no-op edit/delete (tick not found) does not trigger an unnecessary re-slice.

## Verification

- Canonical build (j6): exit 0; OWzxSlicer.exe + all test targets linked clean.
- PrepareSceneDataTests: passed.
- PartPlateTests: passed.
- ViewModelSmokeTests: passed.
- QmlUiAuditTests: passed (incl. new `customGcodeWritebackAndResliceWired` slot, 85 passed 0 failed).
- PreviewParserTests: passed.
- APP_RUNNING_PID=876 (app launched live).
- Source audit: `grep -c static_cast<...Type>` = 0; `plates_custom_gcodes` referenced 4x.

## Carry-Forward
- 5-type coverage + drag relocation → Phase 119 (TICK-04, TICK-05).
