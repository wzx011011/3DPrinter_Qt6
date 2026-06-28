# Phase 38 Research: Prepare Readiness and Slice Invalidation

**Researched:** 2026-06-29
**Status:** Complete

## Source Truth

### OrcaSlicer Slice Readiness Model

- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp`
  - `can_slice()` returns `m_ready_for_slice && !m_apply_invalid`.
  - `is_slice_result_valid()` is a separate durable result-valid flag.
  - `is_slice_result_ready_for_print()` requires a valid result plus non-error G-code result checks.
  - `is_slice_result_ready_for_export()` additionally requires printable instances.
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp`
  - `PartPlate::update_slice_result_valid_state(false)` sets the plate result invalid and resets slice percent.
  - `PartPlateList::invalid_all_slice_result()` invalidates every plate.
  - all-ready helpers skip empty/unprintable plates but require at least one ready result.
- `third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp`
  - `reset()` clears export state and invalidates all process steps.
  - `apply()` resets G-code preview data when print application invalidates G-code export state.
  - export finalization is a separate step from slice readiness.
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`
  - plate selection and many edit paths call `update_slice_result_valid_state(false)` when the result no longer matches current data.
  - Preview/export paths consult current-plate result validity rather than only checking for a path string.

## Local State Today

- `EditorViewModel` exposes `canRequestSlice`, `hasSliceResult`, `sliceActionLabel`, `sliceActionHint`, and `isPlateSliced`.
- `hasSliceResult()` is currently tied to a non-empty `SliceService::outputPath`, no active slicing, and `m_sliceResultPlateIndex == currentPlateIndex`.
- `m_slicedPlateIndices` tracks a loose "plate has been sliced" flag but does not distinguish valid, stale, missing, preview availability, export availability, or disabled reasons.
- `invalidateSliceResultsForCurrentPlate()` removes only the current plate from `m_slicedPlateIndices`; it does not remove `SliceService` per-plate result metadata or clear current output when the current plate result becomes stale.
- `SliceService` now has `clearResults`, `resultChanged`, `sliceResultCleared`, `hasPlateResult`, and per-plate statistics from Phase 37.
- Prepare QML routes slice, Preview, and export actions to `EditorViewModel` but largely trusts existing coarse booleans.

## Implementation Direction

- Add an explicit readiness/result model in `EditorViewModel`, backed by small helper methods and stable Q_PROPERTY values.
- Keep `SliceService` as the owner of output paths and result metadata, but let `EditorViewModel` decide whether those results are current for the selected plate.
- Replace the ambiguous `isPlateSliced()` contract with a richer status API while keeping it as compatibility shorthand for "valid result".
- Invalidate specific plates where possible and all plates for global config/bed/unknown scope changes.
- On invalidating the current output plate, clear or remove the corresponding `SliceService` result so Preview/export cannot use stale paths.

## Test Strategy

- Use `tests/E2EWorkflowTests.cpp` for real STL slice result transitions because it already has deterministic `applyMinimalPrinterConfig()`.
- Use `tests/ViewModelSmokeTests.cpp` for fast viewmodel-level readiness and invalidation behavior that does not need a full real slice.
- Use `tests/QmlUiAuditTests.cpp` only when QML bindings or normal action routing changes.
- Required red tests:
  - successful slice makes Preview/export available for that plate;
  - switching to a different unsliced plate disables Preview/export and shows a reason;
  - toggling object printable, moving objects between plates, changing plate printable/settings, and changing bed settings invalidate affected results;
  - `switchToPreview()` and `requestExportGCode()` refuse stale/missing results with a user-visible reason.

## Risks

- `SliceService::removePlateResult()` currently removes statistics but not the global `outputPath_`; invalidating the current plate must avoid leaving the path exportable.
- Some edit methods already call `invalidateSliceResultsForCurrentPlate()` before/after service changes. Refactoring must avoid double signals or clearing unrelated valid plate state unnecessarily.
- Full all-plate slicing and export semantics belong to Phase 39/42; Phase 38 should expose availability accurately without over-implementing those lifecycle features.

## Research Complete

The phase can proceed with one implementation plan covering TDD regression tests, readiness state API, invalidation plumbing, Prepare QML binding, and verification.
