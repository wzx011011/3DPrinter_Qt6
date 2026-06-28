---
phase: 37
plan: 01
type: implementation
wave: 1
depends_on: []
files_modified:
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/core/viewmodels/EditorViewModel.cpp
  - src/core/viewmodels/PreviewViewModel.h
  - src/core/viewmodels/PreviewViewModel.cpp
  - src/qml_gui/pages/PreparePage.qml
  - tests/E2EWorkflowTests.cpp
  - tests/QmlUiAuditTests.cpp
autonomous: true
requirements_addressed: [IMP-01, IMP-02, IMP-03, IMP-04, IMP-05, IMP-06]
source_truth:
  - third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp::load_files
  - third_party/OrcaSlicer/src/libslic3r/Format/3mf.cpp
  - third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.cpp
---

# Phase 37 Plan: Complete Import and Project Restore

<objective>
Make import/project restore state-consistent across the local workflow. Import entry points must use the same backend semantics, advertised formats must be consistent and real/blocked, successful imports must refresh Prepare state, and any new import or workspace clear must invalidate stale slice/Preview/export state immediately.
</objective>

<truths>

- D-37-01: Source truth is OrcaSlicer `Plater::load_files` plus libslic3r `Model::read_from_file` / `Model::read_from_archive`; Qt code must preserve those import semantics rather than invent product behavior.
- D-37-02: All advertised import filters must include the same model formats unless a format is explicitly blocked or hidden.
- D-37-03: Import/project load success must refresh object entries, active plate membership, mesh payload, selection, fit hint, loading progress, and visible Prepare state.
- D-37-04: Import/project load failure must clear loading state and report an actionable error without exposing stale Prepare state as current.
- D-37-05: Starting a new import or clearing the workspace invalidates stale slice, per-plate slice, Preview, and export state.
- D-37-06: 3MF project restore must preserve plate names, locked/printable state, bed type, print sequence, spiral mode, filament maps/manual mode, thumbnails, and embedded config where local representations exist.

</truths>

<tasks>

## Task 1 - Add Slice/Preview Result Invalidation API

type: tdd
files:
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Add a public `SliceService::clearResults()` API that clears current output metadata and per-plate result cache.
- Emit a dedicated signal when stored slice output is cleared, so non-slicing consumers can clear derived state.
- Make `PreviewViewModel` respond to that signal by clearing packed G-code preview data, legend, layer/move counters, timing/statistics, playback state, and tool marker state.
- Preserve existing `sliceFinished` parsing behavior for successful slice and standalone G-code load.

verify:
- Add an E2E test that produces or loads Preview G-code data, calls the new clear path through import/workspace semantics, and asserts `SliceService::outputPath`, `EditorViewModel::hasSliceResult`, `PreviewViewModel::gcodePreviewData`, `moveCount`, and `layerCount` are reset.

acceptance_criteria:
- No stale G-code preview remains visible after importing another file or clearing the workspace.
- Export cannot use an old `SliceService::outputPath` after import invalidation.

## Task 2 - Wire Import and Workspace Clear to Invalidation

type: implementation
files:
- `src/core/viewmodels/EditorViewModel.cpp`

action:
- Call the new slice clear API when `EditorViewModel::loadFile()` successfully starts an import.
- Call the same clear API from `clearWorkspace()` and `refreshAfterLoad()`.
- Keep selection, mesh cache, fit hint, group collapse, and per-plate sliced state reset behavior aligned with existing code.
- Avoid clearing old state on a path normalization failure or rejected load request that never starts.

verify:
- Existing ViewModel/E2E tests still pass.
- New E2E coverage observes invalidation through `EditorViewModel::loadFile()`.

acceptance_criteria:
- Import success/failure cannot leave stale slice/Preview/export state.
- `canRequestSlice()` becomes true again after a valid import with printable objects.

## Task 3 - Normalize Advertised Import Formats

type: implementation
files:
- `src/qml_gui/pages/PreparePage.qml`
- `tests/QmlUiAuditTests.cpp`

action:
- Bring Prepare page model-open filters into alignment with the topbar and Project page by including STEP/STP alongside 3MF/STL/OBJ/AMF.
- Add a static QML audit that asserts normal import entry points advertise the same expected model extensions and continue routing into backend/viewmodel import calls.

verify:
- `QmlUiAuditTests` passes.

acceptance_criteria:
- `.3mf`, `.stl`, `.obj`, `.amf`, `.step`, `.stp` are consistently visible in normal model import entry points.
- Unsupported formats are not falsely presented through one entry point but missing from another.

## Task 4 - Verify 3MF Restore Coverage and Record Remaining Source-Truth Gaps

type: verification
files:
- `tests/ViewModelSmokeTests.cpp`
- `tests/PartPlateTests.cpp`
- `.planning/phases/37-complete-import-and-project-restore/37-VERIFICATION.md`

action:
- Reuse existing 3MF restore tests for plate metadata, thumbnails, filament maps, and embedded config.
- If any upstream 3MF metadata is represented in OrcaSlicer but missing in OWzx, record it as Real/Hybrid/Blocked/Future in verification notes rather than silently claiming parity.

verify:
- Run canonical build/verification command.

acceptance_criteria:
- Phase verification explicitly states which 3MF restore fields are implemented and which are deferred.
- Real STL and real 3MF coverage are part of automated verification or explicitly reported if a fixture dependency blocks execution.

</tasks>

<verification>

1. Run focused tests while developing:
   - `ctest --test-dir build -R "E2EWorkflowTests|QmlUiAuditTests" --output-on-failure` after a successful build exists.
2. Run the canonical project verification before closeout:
   - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
3. Inspect `build/Testing/Temporary/LastTest.log` if any CTest target fails.
4. Confirm no noncanonical build directory was created.

</verification>

<success_criteria>

- Importing a real STL fixture refreshes Prepare object count, mesh payload, fit hint, and active plate data.
- Real 3MF project restore coverage is verified for implemented local fields or documented with precise gaps.
- All normal model import entry points advertise consistent model formats and route to backend import semantics.
- Starting import or clearing workspace clears old slice output, per-plate slice result cache, Preview packed G-code data, and export source path.
- Canonical verification passes or any failure is reported with a concrete blocker and next action.

</success_criteria>
