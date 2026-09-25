---
phase: 39-complete-slicing-and-reslicing-state-machine
plan: 01
subsystem: slicing-state-machine
tags: [qt, slicer, gcode, preview, multi-plate, state-machine]
requires:
  - phase: 38-prepare-readiness-and-slice-invalidation
    provides: Prepare readiness, stale result gating, and per-plate result status UI contract
provides:
  - SliceService per-plate output path and source metadata
  - terminal cleanup for cancelled, failed, and reused G-code jobs
  - active result restoration when selecting plates after slice-all
  - E2E coverage for previous-G-code reuse, cancellation, and slice-all skipped plates
affects: [phase-40-preview-data, phase-42-gcode-export, phase-43-e2e-verification]
tech-stack:
  added: []
  patterns:
    - C++ services own durable slicing lifecycle state; QML/viewmodels query the service contract
key-files:
  created:
    - .planning/phases/39-complete-slicing-and-reslicing-state-machine/39-REVIEW.md
    - .planning/phases/39-complete-slicing-and-reslicing-state-machine/39-SUMMARY.md
    - .planning/phases/39-complete-slicing-and-reslicing-state-machine/39-VERIFICATION.md
  modified:
    - src/core/services/SliceService.h
    - src/core/services/SliceService.cpp
    - src/core/viewmodels/EditorViewModel.cpp
    - tests/E2EWorkflowTests.cpp
key-decisions:
  - "SliceService stores per-plate output metadata, including whether a result came from a model slice or previous G-code reuse."
  - "Cancellation remains an active job request until the worker terminal path clears state."
  - "Switching to a plate with no valid output clears the active service output path to prevent stale Preview/export."
patterns-established:
  - "Use activatePlateResult(index) when the selected plate changes so Preview/export target the selected plate result."
  - "Use hasPlateResult(index) as an existence-checked result contract, not just a map membership check."
requirements-completed: [SLICE-01, SLICE-02, SLICE-03, SLICE-04, SLICE-05, SLICE-06]
completed: 2026-06-29
---

# Phase 39: Complete Slicing and Reslicing State Machine Summary

**The slicing service now has coherent per-plate result ownership for generated G-code, reused G-code, cancellation, failure, and slice-all selection.**

## Accomplishments

- Extended `PlateSliceResult` with output path and result source metadata.
- Added `SliceService::ResultSource`, `plateOutputPath(index)`, `plateResultSource(index)`, and `activatePlateResult(index)`.
- Hardened `startSlice()` terminal paths so success stores per-plate output metadata and failure/cancellation clear the active target result.
- Changed `cancelSlice()` to request cancellation without reopening the slice gate before worker cleanup.
- Implemented previous-G-code reuse as a distinct valid result source with the reused file as active/per-plate output.
- Updated `EditorViewModel` to activate the selected plate's stored result and to validate plates through service-owned per-plate metadata plus stale status.
- Added E2E tests for previous-G-code reuse, cancellation cleanup, and slice-all valid/skipped plate behavior.

## Task Commits

1. **Task 1: RED slicing state-machine tests** - `897dce8` (`test(39-01)`)
2. **Tasks 2-5: Slicing state metadata and reuse handling** - `9a48d0e` (`feat(39-01)`)

## Files Created/Modified

- `src/core/services/SliceService.h` - adds result source enum, per-plate output/source API, active target tracking helpers.
- `src/core/services/SliceService.cpp` - implements terminal cleanup, per-plate result storage, previous-G-code reuse metadata, cancellation behavior, and active result restoration.
- `src/core/viewmodels/EditorViewModel.cpp` - activates selected plate results and checks per-plate validity without relying on one global active result.
- `tests/E2EWorkflowTests.cpp` - covers reused G-code, cancelled slices, and slice-all per-plate outputs/skipped plate clearing.
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-REVIEW.md` - code review report.
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-VERIFICATION.md` - verification evidence.

## Deviations from Plan

- `EditorViewModel.h`, `PreviewViewModel.cpp`, and `tests/ViewModelSmokeTests.cpp` did not need changes; existing public APIs were enough once `SliceService` owned per-plate output metadata.
- Full terminal-state enums for failed/cancelled per-plate metadata were not added because invalid terminal states intentionally remove per-plate output metadata rather than persist invalid entries.

## Residual Risk

- Preview rendering/data semantics are still Phase 40/41 scope; this phase only guarantees that Preview receives a valid selected G-code path.
- Local export naming/finalization policy remains Phase 42 scope.

## Next Phase Readiness

Phase 40 can consume `SliceService::outputPath()` and per-plate activation knowing the active path belongs to the selected valid plate and cannot be a skipped, failed, cancelled, or stale result.

