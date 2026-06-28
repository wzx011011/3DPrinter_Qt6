---
phase: 40
plan: 01
type: implementation
wave: 1
depends_on: [39]
files_modified:
  - src/core/viewmodels/PreviewViewModel.h
  - src/core/viewmodels/PreviewViewModel.cpp
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - tests/E2EWorkflowTests.cpp
autonomous: true
requirements_addressed: [PREVIEW-01, PREVIEW-02, PREVIEW-03, PREVIEW-04]
source_truth:
  - third_party/OrcaSlicer/src/libslic3r/GCode/GCodeProcessor.hpp
  - third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/IMSlider.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/TickCode.hpp
---

# Phase 40 Plan: Complete Preview Data and Upstream View Semantics

<objective>
Make Preview state fully data-correct for the local workflow: the active plate's valid G-code result drives parser metadata, view modes, legends, statistics, layer/move sliders, ticks/custom-code markers, and marker tooltips without stale data after plate switches, reslices, imports, or failures.
</objective>

<truths>

- D-40-01: `PreviewViewModel` owns parsed Preview data and QML-facing semantics; QML must only bind and render state.
- D-40-02: The active `SliceService::outputPath()` is the source of truth for model-generated and previous-G-code Preview data.
- D-40-03: Result switching through `SliceService::resultChanged` is as important as `sliceFinished`; Preview must rebuild or clear on both.
- D-40-04: Parser data must be stored per segment and reused for view-mode recoloring; view-mode changes must not reparse text.
- D-40-05: Existing `GCV1` remains the preferred renderer contract; Phase 40 should fill its current fields correctly unless a required field forces an extension.
- D-40-06: Phase 40 does not fix D3D11 QRhi interaction blanking; it provides coherent data so Phase 41 can focus on rendering stability.
- D-40-07: Tests are RED-first and use deterministic G-code fixtures for parser semantics.

</truths>

<tasks>

## Task 1 - Add RED Tests for Preview Active Result and Parser Semantics

type: tdd
files:
- `tests/E2EWorkflowTests.cpp`

action:
- Add failing tests that cover Phase 40 public behavior:
  - Preview rebuilds from `SliceService::outputPath()` when a valid previous-G-code result becomes active through `resultChanged`, not only through `sliceFinished`.
  - Preview clears when the active output path is cleared, invalid, failed, or switched to a plate with no valid result.
  - Parser handles real commands for `M106`, `M107`, `M104`, `M109`, `M204`, `M82`, `M83`, `G92 E`, `Tn`, width/height comments, layer/Z markers, elapsed-time comments, and custom-code marker comments.
  - Marker tooltip remains valid at the final move cursor position.
  - View modes produce correct legend type and non-degenerate gradient/extruder ranges for speed, width, fan, temperature, acceleration, layer time, tool, and feature type.

verify:
- Run the canonical script once to capture the expected RED failure:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

acceptance_criteria:
- Tests fail for missing or incorrect Phase 40 behavior before production code is changed.
- Tests do not rely on renderer pixel output or noncanonical build directories.

## Task 2 - Wire Preview Lifecycle to Active Slice Results

type: implementation
files:
- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`

action:
- Add a small private Preview lifecycle helper that rebuilds from the current valid `SliceService::outputPath()` or resets if it is empty/invalid.
- Connect `SliceService::resultChanged` to that helper so per-plate activation and previous-G-code reuse update Preview immediately.
- Keep `sliceFinished` as the place to update estimated time labels, but avoid double-building stale paths.
- Reset full Preview state on `sliceResultCleared` and `sliceFailed`.
- Preserve standalone `loadGCodeForPreview()` behavior for tests/direct preview.

verify:
- Run focused E2E tests for active result switching and stale reset.

acceptance_criteria:
- Switching to a plate with a valid output loads that plate's Preview data.
- Switching to a missing/invalid output clears `gcodePreviewData`, layers, moves, marker, stats, and ticks.
- Failed/cancelled/invalidation flows cannot leave the previous Preview visible through the viewmodel.

## Task 3 - Complete Parser Command and Layer Semantics

type: implementation
files:
- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Parse command-side fan speed:
  - `M106 S...` as percentage from 0-255 or 0-100 input;
  - `M107` as fan off.
- Parse command-side temperatures:
  - `M104 S...` and `M109 S...` into current temperature.
- Parse acceleration with the existing `M204` support and verify `S/P/T` variants that Orca emits.
- Parse width and height/layer height from common comments (`WIDTH`, `HEIGHT`, `;LAYER_HEIGHT`, `;HEIGHT`, `;Z`, `;LAYER_CHANGE`) and store height separately from absolute Z.
- Preserve extrusion semantics for absolute/relative E and `G92 E`.
- Derive per-move extrusion delta, path length, and volumetric/flow value where possible.
- Keep Z-hop travel out of selectable printed layer count.

verify:
- Parser fixture tests pass for move counts, layer count, metadata values through marker tooltip, and view-mode ranges.

acceptance_criteria:
- `PREVIEW-02` command coverage is represented in parsed data.
- Height mode uses layer height when known, not just raw Z position.
- Flow mode uses derived extrusion/path data where available instead of `feedrate * width` alone.

## Task 4 - Align View Modes, Legends, Statistics, and Marker Data

type: implementation
files:
- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Extend `StoredSegment` only as needed for height, extrusion delta, path length, volumetric flow, custom-code association, and cumulative/duration time values.
- Make `recolorAndPackSegments()` use real stored values for each required mode:
  - feature type;
  - height/layer height;
  - width;
  - feedrate/speed;
  - fan speed;
  - temperature;
  - tool/extruder;
  - filament/color;
  - flow/volumetric rate;
  - layer time and log layer time;
  - acceleration.
- Make legend type, gradient min/max labels, and discrete/extruder rows reflect the visible segments and active mode.
- Make marker tooltip values clamp to the active segment at `currentMove == moveCount`.
- Ensure play timer updates marker data as it advances.
- Keep `GCV1` parsing in `RhiViewportRenderer` synchronized if any field shape changes; otherwise leave renderer untouched.

verify:
- Tests assert public legend/stat/marker APIs rather than renderer internals.
- `git diff --check`.

acceptance_criteria:
- `PREVIEW-03` and `PREVIEW-04` view modes and data surfaces are internally consistent.
- No mode uses a misleading proxy when real parsed data is available.

## Task 5 - Add Parsed Tick and Custom-code Marker Data

type: implementation
files:
- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Parse common custom-code markers into local tick data when they appear in G-code comments:
  - pause print;
  - color change / filament change;
  - tool change markers;
  - custom G-code markers with extra text.
- Convert marker Z/layer references to the closest known print layer/tick.
- Keep manual tick APIs functional and sorted with parser-backed ticks.
- Clear parser-backed ticks on preview reset/reparse so old markers do not leak across files.

verify:
- Fixture test validates tick count, sorted tick positions, type, extruder, color/extra fields, and reset behavior.

acceptance_criteria:
- Tick/custom-code markers visible in QML come from the active Preview result.
- Manual APIs and parsed markers do not leave duplicate stale state after reload.

## Task 6 - Documentation, Review, and Phase Closeout

type: verification
files:
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-SUMMARY.md`
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-VERIFICATION.md`
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-REVIEW.md`
- `.planning/STATE.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`

action:
- Run canonical verification and `git diff --check`.
- Review changed code for stale preview leaks, parser regressions, misleading legend values, invalid binary payload alignment, missing signals, and tests that overfit implementation details.
- Record verification evidence, implemented requirements, commits, and any explicit Phase 41 carry-forward rendering issues.
- Mark `PREVIEW-01` through `PREVIEW-04` complete only if tests and review pass.

verify:
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- `git diff --check`

acceptance_criteria:
- Canonical verification passes.
- Phase artifacts and requirements traceability match the implemented behavior.

</tasks>

<verification>

1. Add RED tests before production changes.
2. Capture expected failing canonical verification output.
3. Implement lifecycle, parser, view-mode, legend/stat, marker, and tick changes.
4. Re-run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
5. Run `git diff --check`.
6. Perform code review and write `40-REVIEW.md`.
7. Write `40-SUMMARY.md` and `40-VERIFICATION.md`.
8. Use `gsd-sdk query phase.complete 40` only after verification passes.

</verification>

<success_criteria>

- Preview data follows the active valid output path for current plate and previous-G-code reuse.
- Empty, invalid, failed, cancelled, imported, or plate-missing states clear stale Preview data.
- Parser fixtures cover Orca-style command and comment semantics for moves, extrusion, layers, view-mode values, elapsed time, and custom markers.
- View-mode changes recolor from stored data and produce matching legends/statistics.
- Layer/move sliders, marker tooltip, role/per-extruder usage, layer-time chart, and tick data match the active Preview result.
- Canonical verification passes.

</success_criteria>
