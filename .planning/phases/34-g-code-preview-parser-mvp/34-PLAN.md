---
phase: 34
plan: 01
type: tdd
wave: 1
depends_on: []
files_modified:
  - tests/E2EWorkflowTests.cpp
  - src/core/viewmodels/PreviewViewModel.h
  - src/core/viewmodels/PreviewViewModel.cpp
autonomous: true
requirements:
  - GCODE-01
  - GCODE-02
  - GCODE-03
  - TEST-02
---

# G-code Preview Parser MVP Implementation Plan

<objective>
Make `PreviewViewModel` parse common slicer G-code into correct preview metadata and a travel-toggle-aware `GCV1` payload.
</objective>

<tasks>

## Task 1: Parser fixture regression

**Files:** `tests/E2EWorkflowTests.cpp`

**Action:**
- Add a generated temporary `.gcode` fixture containing:
  - `M82` absolute extrusion.
  - `G92 E0` reset.
  - `M83` relative extrusion.
  - `G0`/`G1` travel and extrusion moves.
  - `T0`/`T1` tool changes.
  - Z moves across at least two layers.
- Load it with `SliceService::loadGCodeFromPrevious`.
- Assert `PreviewViewModel` exposes:
  - non-empty `GCV1` payload,
  - expected extrude/travel split,
  - tool change count and extruder count,
  - layer count greater than or equal to two,
  - travel-hidden payload contains fewer packed segments than travel-visible payload.

**Verify:**
- Run canonical verification command and confirm the new test fails before production changes.

## Task 2: Parser state machine fix

**Files:** `src/core/viewmodels/PreviewViewModel.cpp`, `src/core/viewmodels/PreviewViewModel.h`

**Action:**
- Track absolute vs relative extrusion mode.
- Apply `G92 E` to the parser's current extrusion reference.
- Treat relative positive E values as extrusion deltas independent of previous absolute E.
- Preserve tool change tracking and per-extruder usage accumulation.
- Store whether each parsed segment is travel or extrusion so downstream packing can filter travel moves.

**Verify:**
- Run the focused E2E parser test through the canonical build pipeline.

## Task 3: Travel visibility payload filter

**Files:** `src/core/viewmodels/PreviewViewModel.cpp`, `src/core/viewmodels/PreviewViewModel.h`

**Action:**
- Make `recolorAndPackSegments` skip travel segments when `showTravelMoves` is false.
- Keep total `moveCount`, `extrudeMoveCount`, and `travelMoveCount` as parsed-file statistics.
- Keep tool marker state coherent by using stored segment travel/extrusion classification.

**Verify:**
- Parser fixture test passes with travel visible and hidden.
- Existing real sliced preview tests still pass.

</tasks>

<verification>
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- Confirm `E2EWorkflowTests::test_preview_parser_handles_extrusion_modes_and_travel_filter` passes.
- Confirm existing Phase 33 slice-to-preview test still passes.
</verification>

<success_criteria>
- `GCODE-01`: absolute extrusion, relative extrusion, reset, Z layers, and tool changes are parsed.
- `GCODE-02`: preview payload, layer/move counts, and ranges remain populated from G-code.
- `GCODE-03`: line type/tool distinctions remain available and travel visibility changes packed payload.
- `TEST-02`: deterministic parser fixture regression exists and passes.
</success_criteria>
