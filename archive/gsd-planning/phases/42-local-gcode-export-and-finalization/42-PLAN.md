---
phase: 42
plan: 01
type: implementation
wave: 1
depends_on: [41]
files_modified:
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/core/viewmodels/EditorViewModel.h
  - src/core/viewmodels/EditorViewModel.cpp
  - src/qml_gui/pages/PreparePage.qml
  - src/qml_gui/panels/SliceProgress.qml
  - tests/E2EWorkflowTests.cpp
  - tests/QmlUiAuditTests.cpp
autonomous: true
requirements_addressed: [EXPORT-01, EXPORT-02, EXPORT-03, EXPORT-04, EXPORT-05, EXPORT-06]
source_truth:
  - third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp
  - third_party/OrcaSlicer/src/libslic3r/Print.cpp
---

# Phase 42 Plan: Local G-code Export and Finalization

<objective>
Complete the local `.gcode` export workflow for the current valid plate and all printable valid plates, with source-truth-aligned destination selection, safe finalization, same-path protection, deterministic naming, status/progress reporting, and regression coverage.
</objective>

<truths>

- D-42-01: Export is only available for valid, non-stale slice results. Missing, stale, failed, cancelled, or actively slicing states must not export.
- D-42-02: Export means writing a user-selected final destination, not reusing or overwriting the generated temporary/source G-code path.
- D-42-03: Source/target same-path must fail safely and must not delete the generated source.
- D-42-04: Final output must be verified after copy/finalization before success is reported.
- D-42-05: Current-plate and all-printable-plate export share the same safe copy/finalize helper.
- D-42-06: QML remains presentation only; service/viewmodel own naming, validation, and export behavior.
- D-42-07: Tests are RED-first and use deterministic temp paths/direct fixture files where full slicing is not necessary.

</truths>

<tasks>

## Task 1 - Add RED Export Safety and UI Wiring Tests

type: tdd
files:
- `tests/E2EWorkflowTests.cpp`
- `tests/QmlUiAuditTests.cpp`

action:
- Add E2E tests for `SliceService` export safety:
  - empty target returns false and leaves source untouched;
  - source/target same path returns false and leaves source untouched;
  - target that is an existing directory returns false;
  - exporting to an existing different file replaces it safely and verifies non-empty output;
  - all-plate export writes deterministic per-plate files for valid plate results only.
- Add QML audit tests:
  - `PreparePage.qml` export dialog has `fileMode: FileDialog.SaveFile`;
  - `SliceProgress.qml` export button opens/requests the export dialog instead of exporting to `root.outputPath`;
  - export dialog uses a backend-provided default filename or target suggestion.

verify:
- Run focused tests to capture RED:
  - `.\build\E2EWorkflowTests.exe -o build\e2e_phase42_red.txt,txt`
  - `.\build\QmlUiAuditTests.exe -o build\qml_phase42_red.txt,txt`
- If binaries need rebuild, run canonical verification and capture the expected failure.

acceptance_criteria:
- Tests fail for the current blind-copy/export-to-source behavior before production code changes.

## Task 2 - Implement Safe Current-Plate Export Finalization

type: implementation
files:
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/EditorViewModel.cpp`

action:
- Harden `SliceService::exportGCodeToPath()`:
  - reject empty target;
  - reject missing, non-file, or empty source;
  - reject source/target same canonical path;
  - reject directory target;
  - ensure parent directory exists or fail clearly;
  - copy through a unique sibling temp path;
  - verify temp/final output exists and is non-empty;
  - replace existing destination only after temp copy succeeds;
  - update `sliceState_`, `progress_`, `statusLabel_`, `progressUpdated`, `progressChanged`, and `stateChanged`.
- Keep `canExportGCode()` tied to valid current-plate result and not slicing/exporting.
- Preserve generated source output path after export.

verify:
- Run focused E2E export tests.

acceptance_criteria:
- Current-plate export cannot delete its own generated source and cannot report success without a valid destination file.

## Task 3 - Add Default Export Naming and UI Save Flow

type: implementation
files:
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/qml_gui/pages/PreparePage.qml`
- `src/qml_gui/panels/SliceProgress.qml`
- `tests/QmlUiAuditTests.cpp`

action:
- Add a backend method for default `.gcode` filename, using project/object and plate context.
- Expose the default filename through `EditorViewModel`.
- Set `exportGCodeDlg.fileMode: FileDialog.SaveFile`.
- Set export dialog selected/current file to the backend default where Qt Quick Dialogs supports it.
- Change SliceProgress export action so it opens the export dialog via a page signal/callback or passes an explicit user target. It must not use `root.outputPath` as destination.

verify:
- Run QML audit tests.

acceptance_criteria:
- Normal UI export path asks for a destination file and never defaults to the generated source path as the target.

## Task 4 - Implement All-Printable-Plate Export

type: implementation
files:
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `tests/E2EWorkflowTests.cpp`

action:
- Add service API to export one explicit plate result to a target path.
- Add service API to export all stored valid plate results to deterministic files under a selected directory or base path.
- Add viewmodel API for exporting all printable plates.
- Respect locked/non-printable/missing plate semantics by only exporting valid stored results and reporting failure if no valid plate results exist.
- Preserve active current-plate result after all-plate export.

verify:
- Run E2E all-plate export tests.

acceptance_criteria:
- Current plate and all-printable valid plates can be exported locally with deterministic per-plate paths.
- Missing/skipped plates do not create stale exports.

## Task 5 - Notifications, Review, and Phase Closeout

type: verification
files:
- `.planning/phases/42-local-gcode-export-and-finalization/42-REVIEW.md`
- `.planning/phases/42-local-gcode-export-and-finalization/42-SUMMARY.md`
- `.planning/phases/42-local-gcode-export-and-finalization/42-VERIFICATION.md`
- `.planning/REQUIREMENTS.md`
- `.planning/ROADMAP.md`
- `.planning/STATE.md`

action:
- Run:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - `git diff --check`
- Review changes for:
  - same-path deletion risk;
  - missing/stale result export leaks;
  - destination verification gaps;
  - all-plate naming collisions;
  - UI paths that bypass SaveFile;
  - state/progress signal omissions.
- Write review, summary, and verification docs.
- Mark `EXPORT-01` through `EXPORT-06` satisfied only after verification passes.

acceptance_criteria:
- Canonical verification passes.
- Phase 43 can run full import -> slice -> Preview interaction -> export UAT without open P0/P1 export defects.

</tasks>

<verification>

1. Add RED tests before production changes.
2. Capture expected failing output.
3. Implement safe current-plate export.
4. Implement default naming and QML SaveFile flow.
5. Implement all-plate export.
6. Run focused tests.
7. Run canonical verification.
8. Run `git diff --check`.
9. Perform code review and write phase artifacts.
10. Update requirements/roadmap/state after verification passes.

</verification>

<success_criteria>

- Export is blocked for missing, stale, invalid, cancelled, failed, slicing, or exporting states.
- Current-plate export writes a verified non-empty `.gcode` destination and preserves the generated source file.
- Same-path/self-copy attempts fail safely.
- Existing different destination files are replaced only after a successful temp copy.
- All-printable valid plate results export to deterministic per-plate files.
- UI export entry points use a SaveFile destination path and no longer export to `sliceOutputPath` directly.
- Canonical verification passes.

</success_criteria>
