# Phase 38: Prepare Readiness and Slice Invalidation - Context

**Gathered:** 2026-06-29
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 38 delivers the Prepare-side validity contract for the local import-to-G-code workflow. Prepare must expose accurate C++-owned state for whether the current plate can slice, whether Preview/export is available, which per-plate result is valid or stale, and why a user-facing action is disabled. The phase owns invalidation after slice-affecting edits across object, volume, plate, transform, printable, arrangement, bed, filament-map, and relevant config changes. It does not implement the full slicing lifecycle, parser/view-mode expansion, renderer stability work, or final export naming/finalization; those remain Phases 39-42.

</domain>

<decisions>

## Implementation Decisions

### Readiness Ownership

- Put durable readiness and stale-result decisions in C++ (`EditorViewModel` with `SliceService`/`ProjectServiceMock` support), not in QML.
- Model readiness as current-plate and per-plate state: can slice, can preview, can export, has valid result, result is stale, disabled/action reason.
- Align source-truth semantics with OrcaSlicer `PartPlate::is_ready_for_slice`, `PartPlate::is_slice_result_valid`, `BackgroundSlicingProcess`, and Plater action-button updates.
- Keep QML responsible only for binding button enabled states, labels, hints, and status chips to the C++ properties.

### Invalidation Semantics

- Any edit that changes model geometry, object membership, printable state, plate configuration, bed shape, filament map, or relevant preset/config must invalidate affected slice results immediately.
- Prefer precise per-plate invalidation when the affected plate is known; use all-plate invalidation for global config/bed changes or uncertain multi-plate operations.
- Import and workspace reset already clear global results from Phase 37; Phase 38 should preserve and formalize that API rather than adding another parallel clearing path.
- Slice worker cancellation must not reopen a slice gate until the worker completion path closes; preserve the Phase 37 state-machine guard.

### UI Action Contract

- Prepare actions must not be hidden no-ops: slice, preview, export, and slice-all entry points need explicit enabled/disabled reasons.
- `switchToPreview()` must only navigate when a valid current-plate result exists; otherwise it should surface the readiness reason.
- Export availability should be stricter than slice availability: it requires a valid current-plate result, a non-empty output path, and no active slice/export job.
- Plate cards should indicate valid/stale/missing result status rather than a single ambiguous "sliced" dot when the backend can distinguish those states.

### Verification

- Add focused tests before or with implementation for representative invalidation triggers: object printable toggle, transform/edit operation, plate switch, plate printable/locked changes, bed/config changes, and successful slice result restoration.
- Extend existing E2E/ViewModel tests instead of creating a new test harness unless a new seam is clearly needed.
- Add QML audit coverage if Prepare bindings or labels are changed.
- Use the canonical verification command before marking Phase 38 complete.

### The Agent's Discretion

Helper class boundaries, exact enum/property names, and test decomposition are at the agent's discretion as long as the source of truth is C++ and the API is stable for later Phase 39-42 work.

</decisions>

<code_context>

## Existing Code Insights

### Reusable Assets

- `src/core/viewmodels/EditorViewModel.{h,cpp}` already exposes `canRequestSlice`, `hasSliceResult`, `sliceActionLabel`, `sliceActionHint`, `isPlateSliced`, `requestSlice`, `requestSliceAll`, `switchToPreview`, and `requestExportGCode`.
- `src/core/services/SliceService.{h,cpp}` now provides `clearResults`, `resultChanged`, `sliceResultCleared`, per-plate result accessors, and output/statistic labels.
- `src/core/services/ProjectServiceMock.{h,cpp}` exposes plate/object membership, printable flags, scoped config overrides, bed/plate settings, filament maps, and signals for project/plate/load changes.
- `src/qml_gui/pages/PreparePage.qml` already binds to `editorVm` for slice requests, Preview navigation, export dialog routing, and plate-card sliced indicators.
- Existing tests in `tests/E2EWorkflowTests.cpp`, `tests/ViewModelSmokeTests.cpp`, and `tests/QmlUiAuditTests.cpp` already exercise slice output, Preview data reset, and Prepare/QML wiring.

### Established Patterns

- Business and validation rules live in C++ viewmodels/services; QML is presentation/wiring.
- `EditorViewModel::invalidateSliceResultsForCurrentPlate()` exists but is narrow and mostly uses `m_slicedPlateIndices`; Phase 38 should expand this into explicit valid/stale state.
- Phase 37 established `SliceService::clearResults()` plus `sliceResultCleared` as the backend invalidation signal consumed by Preview.
- Current tests use real STL fixture slicing with `applyMinimalPrinterConfig()` to make slice/E2E checks deterministic under libslic3r.

### Integration Points

- Source truth: `third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.*`, `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`, and upstream `PartPlate` slice-result validity methods.
- Local invalidation call sites: object printable/visibility/selection edits, object transform setters, delete/duplicate/move/arrange/split/cut/boolean/drill/text/SVG operations, plate operations, bed setters, plate config setters, filament-map/scoped config setters, and config viewmodel changes.
- Downstream phases: Phase 39 can consume the readiness model for reslice/cancel/failure state; Phase 40-41 can trust Preview availability; Phase 42 can consume export availability and disabled reasons.

</code_context>

<specifics>

## Specific Ideas

- The user wants the full implementation, not MVP, and wants the main import -> slice -> Preview -> export flow to run quickly and reliably.
- The user specifically called out that Preview/Prepare must not rely on software fallback or stale state masking real renderer work.
- Phase 38 should reduce hidden stale-result behavior before deeper slicing, Preview parser, renderer, and export work proceed.

</specifics>

<deferred>

## Deferred Ideas

- Full `BackgroundSlicingProcess` lifecycle parity, previous-G-code reuse semantics, and all-printable-plate slicing are Phase 39.
- Preview parser/view-mode/statistics completion is Phase 40.
- D3D11 Preview interaction stability and buffer lifecycle work are Phase 41.
- Export finalization, naming, and all-plate export are Phase 42.
- Device/cloud/Monitor print-job workflows remain out of v3.4 local workflow scope.

</deferred>
