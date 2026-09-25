---
phase: 38
plan: 01
type: implementation
wave: 1
depends_on: []
files_modified:
  - src/core/viewmodels/EditorViewModel.h
  - src/core/viewmodels/EditorViewModel.cpp
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/qml_gui/pages/PreparePage.qml
  - tests/ViewModelSmokeTests.cpp
  - tests/E2EWorkflowTests.cpp
  - tests/QmlUiAuditTests.cpp
autonomous: true
requirements_addressed: [PREP-01, PREP-02, PREP-03, PREP-04]
source_truth:
  - third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp::can_slice
  - third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp::is_slice_result_valid
  - third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp::is_slice_result_ready_for_export
  - third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp::apply
  - third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp::reslice
---

# Phase 38 Plan: Prepare Readiness and Slice Invalidation

<objective>
Make Prepare accurately report and enforce whether slicing, Preview, and export are valid for the current plate. The backend must distinguish slice readiness, valid/stale/missing per-plate results, Preview availability, export availability, and user-facing disabled reasons. Any slice-affecting edit must invalidate the affected result immediately so stale Preview/export cannot be used.
</objective>

<truths>

- D-38-01: Readiness and stale-result decisions are C++ state owned by `EditorViewModel`/`SliceService`, not QML conditionals.
- D-38-02: Readiness is not the same as result validity: a plate may be sliceable, missing a result, holding a valid result, or holding a stale result.
- D-38-03: Preview requires a valid current-plate result; export additionally requires a valid output path/source file and no active slice/export job.
- D-38-04: Slice-affecting edits must invalidate affected plates immediately; global config/bed/unknown-scope edits invalidate all plate results.
- D-38-05: User actions must never become hidden no-ops. Disabled/no-op slice, Preview, and export attempts must expose a reason through `statusText`/action hints.
- D-38-06: Existing Phase 37 `SliceService::clearResults()` and `sliceResultCleared` remain the single global stale-output clearing path.

</truths>

<tasks>

## Task 1 - Add Red Tests for Readiness and Stale Result Semantics

type: tdd
files:
- `tests/ViewModelSmokeTests.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Add viewmodel tests that describe the desired API for slice readiness, Preview availability, export availability, disabled reasons, and per-plate result status.
- Add an E2E regression that slices a real STL, verifies Preview/export availability, performs a slice-affecting edit, and asserts the result becomes unavailable/stale before any new slice.
- Cover representative triggers: object printable toggle, plate switch to an unsliced plate, plate printable/config change, and bed shape/config change.

verify:
- Run the relevant test target or canonical script enough to observe the new tests fail for missing API/behavior before implementing production code.

acceptance_criteria:
- Tests fail for the expected reason: missing readiness properties or stale-result invalidation behavior.
- Tests do not rely on timing except for existing real slice completion waits.

## Task 2 - Implement C++ Readiness and Result Status API

type: implementation
files:
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`

action:
- Add stable Q_PROPERTY values for current-plate readiness: `canPreview`, `canExportGCode`, `sliceReadinessReason`, `previewActionHint`, `exportActionHint`, and a compact plate result status query.
- Keep `isPlateSliced()` as compatibility shorthand for a valid result, but back it with the new per-plate status.
- Ensure successful slicing marks only the completed result plate as valid and updates current-plate derived properties.
- Extend `SliceService` only where needed to clear/remove plate result metadata and prevent stale `outputPath_` from remaining exportable for an invalidated current plate.
- Preserve worker cancellation semantics from Phase 37.

verify:
- Re-run the red tests from Task 1 until the API-related failures pass.
- Confirm existing slice result propagation tests still pass.

acceptance_criteria:
- Current-plate slice, Preview, and export availability are each independently queryable.
- Disabled reasons distinguish loading, no file, no printable objects, slicing in progress, already valid result, stale/missing result, and missing output path.

## Task 3 - Centralize Invalidation Hooks Across Prepare Mutations

type: implementation
files:
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/core/services/SliceService.cpp`

action:
- Replace ad hoc `m_slicedPlateIndices.remove(...)` usage with helper methods such as current-plate invalidation, specific-plate invalidation, and all-plate invalidation.
- Wire invalidation for object printable changes, object/volume add/delete/transform/arrange operations, move-to-plate operations, plate printable/locked/config changes, bed shape setters, and relevant config/preset changes.
- When invalidating the current plate result, clear Preview/export availability immediately through the existing `SliceService` result-cleared path or a precise per-plate removal path that also guards the current `outputPath_`.
- Preserve unrelated valid plate state when the changed scope is known to be one plate.

verify:
- ViewModel/E2E tests cover each representative invalidation category.
- Manually inspect call sites found by `rg "invalidateSliceResults|m_slicedPlateIndices|setPlate|setObject|arrange|bed"` to ensure no obvious slice-affecting path was missed.

acceptance_criteria:
- Any representative slice-affecting edit clears affected Preview/export availability before a reslice.
- Switching plates shows correct valid/missing/stale state for that plate.

## Task 4 - Bind Prepare UI to Backend Availability and Reasons

type: implementation
files:
- `src/qml_gui/pages/PreparePage.qml`
- `tests/QmlUiAuditTests.cpp`

action:
- Bind Prepare slice/Preview/export buttons and status indicators to the new C++ availability properties and hints.
- Keep QML free of durable readiness logic; it may display strings and enabled states only.
- Update plate-card status indicators to use explicit valid/stale/missing result status where practical.
- Add or extend static QML audit coverage for the new bindings.

verify:
- `QmlUiAuditTests` passes and verifies QML routes through the new backend properties rather than local stale checks.

acceptance_criteria:
- Prepare UI cannot navigate to Preview or open export for stale/missing current-plate results.
- Disabled UI states explain the reason without duplicate QML business logic.

## Task 5 - Full Verification and Phase Closeout

type: verification
files:
- `.planning/phases/38-prepare-readiness-and-slice-invalidation/38-SUMMARY.md`
- `.planning/phases/38-prepare-readiness-and-slice-invalidation/38-VERIFICATION.md`
- `.planning/STATE.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`

action:
- Run focused checks during development and the canonical project verification before claiming completion.
- Record implemented readiness states, invalidation hooks, remaining source-truth gaps, test evidence, and any deviations from plan.
- Mark `PREP-01` through `PREP-04` satisfied only if the verification evidence covers them.

verify:
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- `git diff --check`

acceptance_criteria:
- Canonical verification passes or a blocker is documented with exact failure output and next action.
- Phase 38 summary and verification artifacts exist and match the implemented code.

</tasks>

<verification>

1. Use TDD red checks before implementation for the new readiness/invalidation tests.
2. Run focused test subsets where possible after a successful build exists:
   - `ctest --test-dir build -R "ViewModelSmokeTests|E2EWorkflowTests|QmlUiAuditTests" --output-on-failure`
3. Run the canonical project verification before closeout:
   - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
4. Run `git diff --check`.
5. Confirm no noncanonical build directory was created.

</verification>

<success_criteria>

- Prepare exposes accurate current-plate slice readiness, Preview availability, export availability, and disabled/action reasons.
- Per-plate result status distinguishes valid, stale, and missing enough for QML and later export/slicing phases.
- Representative object, volume, plate, transform, bed, filament/config, and arrangement edits invalidate affected results immediately.
- Switching plates shows correct per-plate status and never exposes stale Preview/export as current.
- QML binds to C++ availability/reason properties and does not implement durable readiness logic.
- Automated tests cover the main invalidation categories and canonical verification passes.

</success_criteria>
