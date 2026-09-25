---
phase: 39
plan: 01
type: implementation
wave: 1
depends_on: [38]
files_modified:
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/core/viewmodels/EditorViewModel.h
  - src/core/viewmodels/EditorViewModel.cpp
  - src/core/viewmodels/PreviewViewModel.cpp
  - tests/ViewModelSmokeTests.cpp
  - tests/E2EWorkflowTests.cpp
autonomous: true
requirements_addressed: [SLICE-01, SLICE-02, SLICE-03, SLICE-04, SLICE-05, SLICE-06]
source_truth:
  - third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.hpp
  - third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp::update_slice_context
  - third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp::load_gcode_from_file
  - third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp::reslice
---

# Phase 39 Plan: Complete Slicing and Reslicing State Machine

<objective>
Make the Qt slicing lifecycle coherent for the local workflow: current-plate slicing, all printable unlocked plate slicing, cancellation, failure, validation, previous-G-code reuse, per-plate outputs/statistics, and stale-result prevention all transition through explicit C++ state so Prepare and Preview never see exportable stale results.
</objective>

<truths>

- D-39-01: `SliceService` is the owner of active job metadata, terminal job state, and per-plate generated/reused output metadata.
- D-39-02: `EditorViewModel` remains the owner of Prepare readiness and stale/missing/valid UI contract, using `SliceService` result metadata instead of duplicating worker details.
- D-39-03: Cancellation is a requested state while the worker is alive; `slicing()` remains true until worker terminal handling closes.
- D-39-04: Failure, cancellation, and failed previous-G-code reuse clear the active target result and cannot leave `outputPath()` exportable.
- D-39-05: Previous-G-code reuse is a valid result source but not a model slice; it must be distinguishable for Preview/export follow-up phases.
- D-39-06: Slice-all covers printable unlocked plates only and must retain per-plate output/statistics for every successful plate.
- D-39-07: This phase must not change libslic3r slicing algorithms or add QML business rules.

</truths>

<tasks>

## Task 1 - Add RED Tests for Terminal Job State and Reuse

type: tdd
files:
- `tests/ViewModelSmokeTests.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Add focused tests for the API and behavior Phase 39 needs before production changes:
  - a failed slice attempt clears active output and blocks `EditorViewModel::canPreview()` / `canExportGCode()`;
  - cancellation does not create a valid/exportable result and leaves no active output after the worker terminal callback;
  - `loadGCodeFromPrevious()` produces a valid current-plate reused result, distinguishes the source from a model slice, and lets `PreviewViewModel` rebuild from the reused file;
  - slice-all skips locked/non-printable plates and stores output metadata for all successful target plates.
- Prefer API-driven tests that fail for missing methods/properties before behavior tests where possible.

verify:
- Run the canonical script once to capture the RED failure:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

acceptance_criteria:
- The first run fails for expected missing API or incorrect terminal-state behavior.
- Tests do not require live hardware, network, or noncanonical build directories.

## Task 2 - Add Slice Job Metadata and Per-Plate Output API

type: implementation
files:
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`

action:
- Extend `PlateSliceResult` with output path, source kind, terminal status, warning/error text, and total filament where available.
- Add small enums exposed with `Q_ENUM` for result source and terminal state if needed by tests/QML:
  - model slice;
  - previous G-code reuse;
  - failed/cancelled/missing.
- Add public invokable/query methods for:
  - per-plate output path;
  - per-plate result source;
  - active target plate;
  - last error/warning;
  - selecting/restoring the active result for a valid plate.
- Update `clearStoredResult()`, `clearResults()`, `removePlateResult()`, and `clearPlateResults()` so output path and per-plate metadata cannot disagree.

verify:
- Re-run the RED tests enough to confirm compile-time missing-API failures are gone and remaining failures are behavioral.

acceptance_criteria:
- `SliceService` can represent multiple successful plate results without relying on a single global `outputPath_`.
- Active result and per-plate result APIs have deterministic behavior for missing, current, and non-current plates.

## Task 3 - Harden Worker Terminal Handling for Success, Failure, and Cancellation

type: implementation
files:
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/EditorViewModel.cpp`

action:
- Track the active target plate from `startSlice()`/`startSlicePlate()` through worker completion.
- Keep `slicing_` true after `cancelSlice()` until the queued worker terminal lambda runs.
- On cancellation:
  - set terminal state to cancelled;
  - clear active target output/result metadata;
  - emit `sliceResultCleared` when the current active output is removed;
  - emit one coherent user-visible status.
- On validation or processing failure:
  - set terminal state to error;
  - store the last error;
  - clear active target output/result metadata;
  - stop slice-all queue through the existing `EditorViewModel` path.
- On success:
  - store output path and statistics under the target plate;
  - make the target plate the active result if it is the current plate or if slice-all just selected it;
  - emit `sliceFinished` only after metadata is coherent.

verify:
- Run the focused tests from Task 1.
- Inspect all `emit sliceFailed`, `emit sliceFinished`, `emit sliceResultCleared`, and `sliceState_` assignments for consistent terminal transitions.

acceptance_criteria:
- Failed/cancelled jobs cannot leave `canPreview()` or `canExportGCode()` true.
- `slicing()` cannot flip false before the worker terminal path owns cleanup.

## Task 4 - Complete Previous-G-code Reuse Semantics

type: implementation
files:
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Treat `loadGCodeFromPrevious()` as a distinct successful result source for the current plate.
- Validate file existence and libslic3r reprocessing errors before marking the result valid.
- Store the reused file path as the per-plate output path and active output path.
- Ensure `PreviewViewModel` rebuilds from the reused output exactly like a generated output while preserving the source distinction in `SliceService`.
- Keep failed reuse on the same terminal error/cleanup path as failed model slicing.

verify:
- E2E previous-G-code reuse test passes and `PreviewViewModel::moveCount()` or packed preview payload reflects the reused file.

acceptance_criteria:
- Reused G-code can enter Preview when valid.
- Failed reuse cannot leave stale generated output exportable.

## Task 5 - Align Editor Multi-Plate Result Selection with Per-Plate Outputs

type: implementation
files:
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `tests/ViewModelSmokeTests.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Update `hasValidSliceResultForPlate()` to use per-plate output metadata and stale status rather than requiring the single active `resultPlateIndex()` for every queried plate.
- On plate selection changes, activate the selected plate's stored valid result if one exists so Preview/export target the selected plate.
- Keep missing/stale behavior from Phase 38 intact.
- Confirm `requestSliceAll()` still skips locked/non-printable plates and that result status for skipped plates remains missing/stale.

verify:
- Tests prove plate A and plate B can both retain valid results after slice-all, while locked/non-printable plates do not.

acceptance_criteria:
- Switching plates after slice-all exposes the selected plate's correct output path/statistics.
- Skipped plates remain non-previewable and non-exportable.

## Task 6 - Documentation, Review, and Phase Closeout

type: verification
files:
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-SUMMARY.md`
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-VERIFICATION.md`
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-REVIEW.md`
- `.planning/STATE.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`

action:
- Run canonical verification and `git diff --check`.
- Review changed code for source-truth drift, stale-output regressions, racey cancellation, missing signals, and tests that assert implementation details instead of behavior.
- Record commits, implemented requirements, verification evidence, and any carry-forward gaps.
- Mark `SLICE-01` through `SLICE-06` complete only with passing evidence.

verify:
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- `git diff --check`
- Code review file has `status: clean` or documented accepted findings.

acceptance_criteria:
- Canonical verification passes.
- Phase artifacts match the code and requirements traceability.

</tasks>

<verification>

1. Add RED tests before production changes.
2. Capture the expected failing canonical verification output.
3. Implement production changes in small commits.
4. Re-run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
5. Run `git diff --check`.
6. Perform code review and write `39-REVIEW.md`.
7. Write `39-SUMMARY.md` and `39-VERIFICATION.md`.
8. Use `gsd-sdk query phase.complete 39` only after verification passes.

</verification>

<success_criteria>

- Current-plate slicing uses current model, preset injection, bed shape, per-plate config, and normal calibration-neutral state.
- Slice progress, completion, cancellation, failure, and result-clearing states are coherent across `SliceService`, `EditorViewModel`, `PreviewViewModel`, and Prepare actions.
- Stale or failed output paths cannot remain valid for Preview/export.
- Previous-G-code reuse creates a valid, distinguishable reused result and refreshes Preview from that file.
- Slice-all skips locked/non-printable plates and stores per-plate output/statistics for successful plates.
- Warnings/errors surface visibly and block Preview/export when the result is invalid.
- Canonical verification passes.

</success_criteria>
