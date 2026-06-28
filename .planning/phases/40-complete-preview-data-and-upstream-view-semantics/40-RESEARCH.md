# Phase 40 Research: Preview Data and Upstream View Semantics

**Date:** 2026-06-29
**Status:** Complete

## Upstream Source Truth

### `GCodeProcessorResult`

Relevant file:

- `third_party/OrcaSlicer/src/libslic3r/GCode/GCodeProcessor.hpp`

Key data:

- `GCodeProcessorResult::moves` is the source collection for Preview toolpath vertices.
- Each `MoveVertex` stores type, extrusion role, extruder id, color id, position, extrusion delta, feedrate, actual feedrate, width, height, volumetric data (`mm3_per_mm`), travel distance, fan speed, temperature, pressure advance, acceleration, jerk, per-mode time, layer duration, layer id, and print Z.
- Result-level data includes filename, extruder colors, filament diameters/densities/costs, print statistics, custom G-code per print Z, warnings, filament maps, layer filament ranges, and print-time statistics.

Implication for Qt:

- The local parser cannot assume Preview is only XYZ line segments. It must retain enough per-segment metadata to drive view modes, legends, marker tooltips, per-extruder stats, layer times, and tick markers.
- The current `GCV1` payload already contains most local fields needed for Phase 40: position, color, feedrate, fan, temperature, width, layer time, acceleration, extruder id, layer, and move.

### `GCodeViewer`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp`

Key behavior:

- The Preview legend/view selector drives `set_view_type`, visibility reset, and slider update.
- Visible view types in the local workflow include feature type, height, width, speed/feedrate, acceleration, fan speed, temperature, volumetric flow, layer time, tool, and color/filament modes.
- Feature type legend rows show role labels, time, percentage, usage, and display toggles.
- Gradient modes use titles/units such as layer height, line width, speed, fan speed, temperature, volumetric flow rate, and layer time.
- Tool/color modes use extruder-specific color and usage information.

Implication for Qt:

- `PreviewViewModel::buildLegendItems()` must derive from parsed values and report matching units/ranges instead of using unrelated proxies.
- Feature type, tool, and gradient modes need distinct legend types because QML already renders discrete, extruder, and gradient legends differently.

### `IMSlider`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/IMSlider.cpp`

Key behavior:

- `SetLayersTimes()` converts per-layer durations to cumulative layer times for slider labels.
- `SetTicksValues()` converts `custom_gcode_per_print_z` items to tick positions from print Z values.
- Slider labels and hover data use the same layer/move time model as Preview state.

Implication for Qt:

- Local `layerTimeAt()`, `timeAtMove()`, and `currentTime()` should derive from cumulative/duration data consistently.
- Custom G-code/tick markers need stable sorting and conversion from layer/Z metadata into the local tick list.

### `TickCode`

Relevant files:

- `third_party/OrcaSlicer/src/slic3r/GUI/TickCode.hpp`
- `src/core/rendering/TickCodeTypes.h`

Key behavior:

- A tick contains `tick`, type, extruder, color, and extra text.
- Upstream stores ticks sorted by tick position and uses them for color changes, pauses, tool changes, and custom G-code.

Implication for Qt:

- Existing local tick APIs are a usable shape, but Phase 40 needs parser-backed tick/custom-code markers in addition to manual test APIs.

## Current Qt Implementation

### `PreviewViewModel`

Relevant files:

- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`

Current strengths:

- Exposes QML-facing properties for progress, layer/move range, packed preview data, legend items, statistics, view modes, travel/bed/marker toggles, marker tooltip, layer times, role times, extruder usage, and ticks.
- Parses standalone G-code fixtures and rebuilds from `SliceService::outputPath()` after `sliceFinished`.
- Handles `M82/M83`, `G92 E`, tool changes, extrusion/travel classification, Z-hop layer regression, per-extruder usage, role times, and `GCV1` packing.
- Recolors and repacks from stored segment data when view mode or travel visibility changes.

Gaps:

- It does not rebuild on `SliceService::resultChanged`, which makes per-plate active result switching incomplete after Phase 39.
- It does not independently clear stale Preview state on `sliceFailed`.
- It parses fan/temperature only when those tokens appear in comments, not real `M106/M107/M104/M109` commands.
- It stores width but not a reliable height/layer-height value per segment; height mode currently uses raw Z position.
- Volumetric flow uses `feedrate * width`, which is only a rough proxy and should use extrusion delta and path length when derivable.
- Layer-time handling stores a current elapsed value as segment `layer_time`; it should derive per-layer duration and move accumulated time coherently.
- Marker tooltip fails at the UI's valid end position (`currentMove == moveCount`).
- Parsed custom-code markers are absent; only manual tick APIs populate `tickMarks_`.

### `SliceService`

Relevant files:

- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`

Current strengths after Phase 39:

- Stores per-plate output path and result source.
- `activatePlateResult()` can switch the global active output to a selected plate's valid result.
- Clears active output on invalidation/failure/cancellation through existing result signals.

Preview implications:

- `PreviewViewModel` should track active output path changes through `resultChanged`.
- Empty or invalid `outputPath()` after activation/clearing should reset Preview state.

### `RhiViewportRenderer`

Relevant files:

- `src/qml_gui/Renderer/RhiViewportRenderer.h`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`

Current strengths:

- Uses QRhi/D3D11 normal path for Preview through `QQuickRhiItem`.
- Consumes `GCV1` with the same fields packed by `PreviewViewModel`.
- Treats colors as CPU-prebaked, so Phase 40 view-mode correctness can stay in the viewmodel.

Boundary:

- The user-reported blanking after slider/orbit is a Phase 41 renderer lifecycle problem, not a Phase 40 parser problem unless caused by invalid data.

## Implementation Implications

- Keep `PreviewViewModel` as the central public API, but consider small private helpers for parsing commands and segment values to keep risk manageable.
- Add tests around public Preview API rather than renderer internals:
  - active output rebuild/reset on `SliceService::resultChanged`;
  - parser command coverage for fan, temperature, acceleration, width/height, layer markers, elapsed time, tool changes, and custom ticks;
  - view-mode legend type/ranges for gradient and extruder modes;
  - marker values at middle and final move positions.
- Avoid expanding `GCV1` unless a required field cannot be represented by existing segment/color/stat APIs.
- If adding helper methods for tests, keep them QML-safe and source-truth aligned rather than exposing raw parser internals.

## Verification Strategy

- RED tests first in `tests/E2EWorkflowTests.cpp` because existing Preview parser tests live there.
- Prefer deterministic temporary `.gcode` files for parser semantics.
- Use real `SliceService` signals with `loadGCodeFromPrevious()` for active-result lifecycle where full slicing is not necessary.
- Run:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - `git diff --check`
