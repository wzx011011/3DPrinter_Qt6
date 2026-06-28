# Phase 39 Research: Slicing and Reslicing State Machine

**Date:** 2026-06-29
**Status:** Complete

## Upstream Source Truth

### `BackgroundSlicingProcess`

Relevant files:

- `third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp`

Key behavior:

- `start()` refuses to run when the active print is empty unless the current plate already has a valid slice result.
- `stop()` requests cancellation, cancels pending UI tasks, calls `m_print->cancel()`, waits for `STATE_CANCELED`, then resets to `STATE_IDLE`.
- `reset()` stops the worker, resets export scheduling, and invalidates all internal finalize steps.
- `apply()` merges current plate config over the full config before applying the model. If application invalidates print state and G-code has not been exported, the G-code result is reset.
- `validate()` runs before processing and returns blocking errors plus optional warnings.
- `finished()` requires both the print finished state and a non-empty G-code result.
- `schedule_export()` and `reset_export()` invalidate the finalization step so final export cannot use stale output.

### `PartPlate`

Relevant files:

- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp`

Key behavior:

- `PartPlate::update_slice_context()` binds the plate-specific `Print`, `GCodeProcessorResult`, printer technology, current plate pointer, and status callback into `BackgroundSlicingProcess`.
- `PartPlateList::update_slice_context_to_current_plate()` updates the process from the current plate before slicing.
- Previous G-code loading applies the full config with filament maps and updates the plate's G-code result from the file.

### `Plater::reslice`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`

Relevant semantics:

- Reslice is the UI action that applies the current model/config state to the background process and starts work only when the process is idle and the plate can slice.
- Validation, cancellation, and export scheduling flow through `BackgroundSlicingProcess`, not through independent UI flags.

## Current Qt Implementation

### `SliceService`

Relevant files:

- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`

Current strengths:

- Has async worker path using `QtConcurrent::run`.
- Injects bed shape, merged presets, per-plate dynamic config, calibration params, and status callbacks before `Print::process()`.
- Runs `Print::validate()` before processing.
- Exposes progress, output path, estimated time, weight, filament, cost, layer count, and per-plate statistic map.
- Provides `clearResults()`, `removePlateResult()`, `loadGCodeFromPrevious()`, and `exportGCodeToPath()`.

Gaps:

- Public state is incomplete: there is no exposed source kind, active target plate, terminal result outcome, or per-plate output path.
- `cancelSlice()` sets `sliceState_ = Cancelled` immediately while the worker may still be running; upstream keeps the job running until canceled/idle.
- Failure and cancellation paths emit `sliceFailed()` but do not always clear plate metadata for the active target.
- `PlateSliceResult` stores statistics but no output path, source kind, warning/error state, or validity token.
- Previous-G-code reuse is reported through `sliceFinished(QString{})` but is not distinguishable from a model slice and does not expose a reuse source.
- Failed reuse leaves state less coherent than normal failed slicing because it does not set the same terminal `State::Error` fields.

### `EditorViewModel`

Relevant files:

- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`

Current strengths:

- Phase 38 exposes `canPreview`, `canExportGCode`, `sliceReadinessReason`, `previewActionHint`, `exportActionHint`, and `plateSliceResultStatus(index)`.
- Tracks valid and stale plates in `m_slicedPlateIndices` and `m_stalePlateIndices`.
- Invalidates current or affected plates after representative Prepare mutations.
- `requestSliceAll()` already excludes locked and non-printable plates.

Gaps:

- `hasValidSliceResultForPlate()` also requires the single global `SliceService::outputPath()` and `resultPlateIndex()` to match the queried plate, so valid results for non-current completed plates cannot be represented after multi-plate slicing.
- Slice-all terminal failure/cancel behavior clears the queue but does not pair that with precise SliceService result cleanup.
- There is no direct action path for previous-G-code reuse through the editor-level readiness contract.

### `PreviewViewModel`

Relevant files:

- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`

Current strengths:

- Rebuilds Preview data from `SliceService::outputPath()` after `sliceFinished`.
- Resets Preview data on `sliceResultCleared`.
- Stops playback and resets move cursor on `sliceFailed`.

Gaps:

- It cannot distinguish model-generated G-code from previous-G-code reuse.
- It relies on the single global output path rather than per-plate output association.
- Preview data completeness is intentionally deferred to Phase 40.

## Implementation Implications

- Add explicit slice job metadata in `SliceService`: active target plate, source kind, terminal result state, and per-plate output path.
- Keep cancellation asynchronous-visible: mark cancellation requested but keep `slicing_` true until the worker returns.
- On terminal failure/cancel, remove the active target plate result if it was the current result path and emit the existing clearing signals.
- Make `hasPlateResult()` and result accessors reflect per-plate output existence, not just statistics map presence.
- Add an API to switch the current active result to a valid per-plate result when the user changes plates or after slice-all completes, so Prepare/Preview/export can reference the selected plate without stale global state.
- Treat previous-G-code reuse as a successful result source with output path and current plate association, while exposing it distinctly for tests and future Preview copy.

## Verification Strategy

- RED tests first:
  - Failed slice validation clears current output and blocks Preview/export.
  - Cancellation does not create a valid/exportable result and does not reopen the gate before worker closure.
  - Previous-G-code reuse records a valid current-plate reused result and refreshes Preview state from the file.
  - Slice-all only targets printable unlocked plates and preserves per-plate output metadata.
- Canonical verification:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - `git diff --check`
