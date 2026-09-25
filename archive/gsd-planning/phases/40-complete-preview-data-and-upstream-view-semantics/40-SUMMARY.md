---
phase: 40-complete-preview-data-and-upstream-view-semantics
plan: 01
subsystem: preview-data
tags: [qt, qml, preview, gcode, parser, viewmodel]
requires:
  - phase: 39-complete-slicing-and-reslicing-state-machine
    provides: active selected-plate G-code output path and previous-G-code reuse metadata
provides:
  - active-result-driven Preview rebuild/reset
  - Orca-style G-code metadata parsing for local Preview
  - view-mode values and legends derived from stored segment data
  - parsed tick/custom-code markers
  - marker tooltip data at final move position
affects: [phase-41-d3d11-preview-rendering, phase-42-gcode-export, phase-43-e2e-verification]
tech-stack:
  added: []
  patterns:
    - PreviewViewModel owns parser semantics; QML and QRhi renderer consume prepared data
key-files:
  created:
    - .planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-REVIEW.md
    - .planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-SUMMARY.md
    - .planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-VERIFICATION.md
  modified:
    - src/core/viewmodels/PreviewViewModel.h
    - src/core/viewmodels/PreviewViewModel.cpp
    - tests/E2EWorkflowTests.cpp
key-decisions:
  - "Keep GCV1 backward compatible and make PreviewViewModel fill existing fields correctly."
  - "Treat SliceService resultChanged as a Preview rebuild/reset trigger, not just sliceFinished."
  - "Keep renderer stability out of Phase 40 so Phase 41 can focus on D3D11 QRhi interaction defects."
patterns-established:
  - "View-mode recoloring must use stored parsed segment values."
  - "Preview reset clears ticks, marker, stats, packed data, and layer/move state together."
requirements-completed: [PREVIEW-01, PREVIEW-02, PREVIEW-03, PREVIEW-04]
completed: 2026-06-29
---

# Phase 40: Complete Preview Data and Upstream View Semantics Summary

**Preview now follows the selected valid G-code result and exposes parser/view-mode data that matches the local Orca-style Preview workflow.**

## Accomplishments

- Connected `PreviewViewModel` to `SliceService::resultChanged` so active result switching, per-plate activation, and previous-G-code reuse rebuild or clear Preview immediately.
- Hardened Preview stale-state cleanup for invalid output paths, result clearing, and slice failure.
- Added parser support for Orca-style feature tags, width/height metadata, elapsed time, fan commands, temperature commands, acceleration variants, tool changes, color/pause/custom tick markers, and derived volumetric flow.
- Fixed marker tooltip behavior at the valid end-of-move cursor position and during playback.
- Updated view-mode recoloring so feature type, height, width, speed, fan, temperature, tool, flow, layer-time, log layer-time, and acceleration use stored segment data.
- Kept the existing `GCV1` renderer payload shape unchanged.
- Added E2E tests for parser metadata/ticks/view modes and active result switching without a new `sliceFinished` event.

## Task Commits

1. **Task 1: RED Preview data semantics tests** - `e01ceef` (`test(40-01)`)
2. **Tasks 2-5: Preview data semantics implementation** - `1d5db58` (`feat(40-01)`)

## Files Created/Modified

- `src/core/viewmodels/PreviewViewModel.h` - adds active-result sync helper and stored segment fields for height/flow.
- `src/core/viewmodels/PreviewViewModel.cpp` - implements result-driven rebuild/reset, parser metadata, tick parsing, marker clamp, and view-mode data fixes.
- `tests/E2EWorkflowTests.cpp` - adds deterministic parser and active-result switching coverage.
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-REVIEW.md` - code review report.
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-VERIFICATION.md` - verification evidence.

## Deviations from Plan

- `SliceService` did not need code changes; Phase 39's active output/result signals were sufficient.
- `RhiViewportRenderer` did not need changes because the required fields fit the existing `GCV1` payload.

## Residual Risk

- The visible blanking/disappearing Preview after layer slider or camera interaction remains Phase 41.
- Layer-time values are parser-derived from elapsed tags and motion timing where available; exact upstream post-processed timing parity still depends on generated G-code metadata quality.

## Next Phase Readiness

Phase 41 can now treat the Preview payload as coherent active-result data and focus on D3D11 QRhi buffer lifecycle, camera/range updates, and interaction stability.
