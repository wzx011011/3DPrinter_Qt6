# Phase 39: Complete Slicing and Reslicing State Machine - Context

**Gathered:** 2026-06-29
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 39 delivers the C++ slicing lifecycle contract for the local import-to-G-code workflow. It owns current-plate slicing, all printable unlocked plate slicing, cancellation, failure, stale-result prevention, previous-G-code reuse, per-plate result metadata, and warning/error propagation into Prepare and Preview. It does not expand Preview parser/view modes, D3D11 renderer stability, or final export naming/finalization; those remain Phases 40-42.

</domain>

<decisions>

## Implementation Decisions

### Source-Truth Lifecycle Mapping

- Map Qt `SliceService` state to upstream `BackgroundSlicingProcess` concepts: idle/running/canceled/finished/error, per-plate print context, validation before start, and result invalidation after `apply()` changes.
- Keep slicing algorithms in libslic3r unchanged; this phase only changes Qt service/viewmodel lifecycle and state ownership.
- Treat `PartPlate::update_slice_context()` and `PartPlateList::update_slice_context_to_current_plate()` as the source-truth model for binding a plate-specific `Print`, G-code result, status callback, plate config, and current plate before processing.
- Preserve Phase 38 readiness as the public Prepare contract; Phase 39 should make the underlying state machine trustworthy rather than adding QML-side guards.

### Job State and Result Validity

- A slice job must have a clear target plate, source kind, and terminal outcome. Terminal success marks only that target plate valid; terminal failure or cancellation must not leave stale current output or stale per-plate metadata exportable.
- `cancelSlice()` must request cancellation and keep `slicing()` true until the worker closes, matching upstream `BackgroundSlicingProcess::stop()` semantics where cancellation waits for the background thread to reach canceled/idle.
- Failed validation, libslic3r exceptions, missing model input, empty plate, canceled jobs, and previous-G-code reuse failures must all emit coherent state, user-visible message, and result-clearing behavior.
- Existing G-code reuse is a distinct successful source kind: it refreshes Preview/result state from the reused file and must not imply a new model slice occurred.

### Multi-Plate Semantics

- Slice-all queues include printable unlocked plates only and must preserve per-plate output/statistics for each completed plate.
- A failed or canceled plate in slice-all stops the queue and records the terminal state; already completed plate results remain valid unless the failing path is a global invalidation.
- Switching plates after multi-plate slicing should expose only the selected plate's valid result and statistics. A plate with no valid result remains missing/stale and cannot Preview/export.
- Per-plate metadata must include output path association where needed so later Preview/export phases can avoid relying on a single global output path.

### Validation and Warnings

- Pre-slice validation errors block Preview/export and surface as user-visible errors/warnings through `SliceService`, `EditorViewModel`, and existing notification hooks.
- Slice warnings that do not block generation should be retained for Prepare/Preview display if libslic3r exposes them during validation.
- Do not hide invalid results by leaving `sliceState_ = Completed` after an error, cancellation, or failed previous-G-code reuse.

### Verification

- Add RED-first tests for cancellation/result cleanup, failed slice validation/result cleanup, previous-G-code reuse result source, and slice-all printable/unlocked filtering plus per-plate metadata.
- Extend existing `E2EWorkflowTests` and `ViewModelSmokeTests` rather than creating a new harness unless a missing seam is unavoidable.
- Run the canonical verification command before closing the phase.

### The Agent's Discretion

Exact enum names, helper struct names, and whether job metadata lives in `SliceService` only or is mirrored in `EditorViewModel` are at the agent's discretion as long as the C++ API is stable for Phases 40-42.

</decisions>

<code_context>

## Existing Code Insights

### Reusable Assets

- `src/core/services/SliceService.{h,cpp}` already owns asynchronous slicing, `State`, progress, result signals, per-plate statistic map, cancellation flag, previous-G-code reuse, and export copy.
- `src/core/viewmodels/EditorViewModel.{h,cpp}` owns Prepare actions, slice-all queue, Phase 38 valid/stale/missing result status, and action hints.
- `src/core/viewmodels/PreviewViewModel.{h,cpp}` listens to `sliceFinished`, `sliceFailed`, and `sliceResultCleared` to rebuild or reset preview data.
- `src/core/services/ProjectServiceMock.{h,cpp}` exposes current plate, locked/printable state, plate dynamic config, object membership, and `cloneCurrentPlateModel()`.
- `tests/E2EWorkflowTests.cpp` already slices real STL fixtures and validates output path, result propagation, Preview data, import invalidation, and bed-change staleness.
- `tests/ViewModelSmokeTests.cpp` already covers PartPlate slice gates and Phase 38 readiness API.

### Established Patterns

- C++ services/viewmodels own durable workflow decisions; QML binds and displays state only.
- Phase 37 introduced `SliceService::clearResults()` and `sliceResultCleared` as the shared stale-output clearing path.
- Phase 38 introduced `EditorViewModel::plateSliceResultStatus(index)` and per-plate valid/stale tracking; later phases must build on that instead of bypassing it.
- Tests use `applyMinimalPrinterConfig()` and the real `Block20XY_model.stl` fixture to keep libslic3r slices deterministic.

### Integration Points

- Upstream source truth: `third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.{hpp,cpp}`, `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp`, `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp::reslice`.
- Local current-plate slice entry: `EditorViewModel::requestSlice()` -> `SliceService::startSlice()`.
- Local all-plate slice entry: `EditorViewModel::requestSliceAll()` -> `continueSliceAllQueue()` -> `SliceService::startSlicePlate()`.
- Local previous-G-code entry: `SliceService::loadGCodeFromPrevious()`, currently only partially represented as a slice success.
- Local Preview bridge: `PreviewViewModel` currently rebuilds from `SliceService::outputPath()` after `sliceFinished`.

</code_context>

<specifics>

## Specific Ideas

- The user wants the complete implementation, not an MVP, and wants the import -> slice/reslice -> Preview -> export path to become usable quickly.
- The user has emphasized performance and renderer correctness separately; Phase 39 should not mask renderer work with stale or software-fallback state.
- Phase 39 should leave explicit hooks for Phase 40 Preview data completion and Phase 42 export finalization to consume per-plate output/source metadata.

</specifics>

<deferred>

## Deferred Ideas

- Preview parser/view-mode/statistics completeness remains Phase 40.
- D3D11 Preview renderer interaction stability remains Phase 41.
- Export naming, all-plate export finalization, and target-file validation remain Phase 42.
- Device/cloud send/upload workflows remain outside v3.4.

</deferred>
