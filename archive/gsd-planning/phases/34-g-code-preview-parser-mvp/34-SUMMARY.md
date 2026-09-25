---
phase: 34
plan: 01
subsystem: preview-gcode-parser
tags:
  - gcode
  - preview
  - tests
key-files:
  modified:
    - src/core/viewmodels/PreviewViewModel.cpp
    - src/core/viewmodels/PreviewViewModel.h
    - tests/E2EWorkflowTests.cpp
metrics:
  tests_added: 1
---

# Phase 34 Summary: G-code Preview Parser MVP

## Result

Complete.

## Delivered

- `PreviewViewModel` now parses standalone G-code for preview data through `loadGCodeForPreview`.
- Parser supports `G0`/`G1` travel/extrusion split, `M82`, `M83`, `G92 E`, Z layer changes, and `Tn` tool changes.
- `showTravelMoves=false` filters travel segments from the packed `GCV1` payload while preserving parsed move statistics.
- Tool marker extrusion state now uses parser classification instead of treating all moving feedrate segments as extrusion.
- `E2EWorkflowTests::test_preview_parser_handles_extrusion_modes_and_travel_filter` covers absolute extrusion, relative extrusion, reset, travel/extrude counts, layers, tool changes, tool legend, and travel-hidden payload count.

## Commits

| Commit | Description |
|---|---|
| `bda6cee` | Harden G-code preview parser and add parser fixture regression. |

## Deviations

- Initial RED attempt through `SliceService::loadGCodeFromPrevious` triggered an upstream `GCodeProcessor` crash on a minimal hand-written fixture. The test was corrected to target the Qt Preview parser directly via a reusable `PreviewViewModel` parsing entry point.

## Self-Check

PASSED.
