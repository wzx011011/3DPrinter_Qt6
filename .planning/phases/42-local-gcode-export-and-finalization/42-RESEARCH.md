# Phase 42 Research: Local G-code Export and Finalization

**Date:** 2026-06-29
**Status:** Complete

## Upstream Source Truth

### `Plater::export_gcode`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`

Key behavior:

- Returns early if there are no model objects or the current process completed with an error.
- Calls `update_restart_background_process(false, false)` before showing the save dialog so filename placeholders and validation are current.
- Builds a default output filename from `background_process.output_filepath_for_project(...)`.
- Uses a Save File dialog with overwrite prompt.
- Validates filename characters against FAT-disallowed characters.
- Records export status, last output path/directory, and starts `priv::export_gcode(...)`.

Implication for Qt:

- Export must be unavailable for missing/stale/invalid current results.
- The UI must ask the user for a destination path rather than silently using the generated temporary path.
- Default naming should be deterministic and based on current project/plate context.

### `Plater::priv::export_gcode`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`

Key behavior:

- Requires an output path or upload job.
- Schedules the background export path through `BackgroundSlicingProcess`.
- Export progress and completion are surfaced through the UI notification/status flow.

Implication for Qt:

- The local `SliceService` may remain synchronous for current scope, but it must still report status/progress and failure reasons.
- A later async export can reuse the same service API if the state contract is correct.

### `BackgroundSlicingProcess::finalize_gcode` and `export_gcode`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp`

Key behavior:

- Applies final print statistics to the export path.
- Optionally runs post-processing scripts on a copy of generated G-code.
- Copies generated temporary G-code to the selected destination.
- Verifies failure modes and throws explicit `ExportError` for:
  - copy failure;
  - source/target mismatch;
  - rename failure;
  - source/target file open failure;
  - unknown export failure.
- Reports success with the final export path.

Implication for Qt:

- Even if post-processing is out of scope, Qt export must not be a blind overwrite/copy.
- The destination file should be created via a temporary sibling and verified.
- Source/target same-path must be rejected because it cannot represent a user export and can destroy the generated source.

### `PrintStatistics::finalize_output_path`

Relevant file:

- `third_party/OrcaSlicer/src/libslic3r/Print.cpp`

Key behavior:

- Resolves filename placeholders after G-code statistics are available.

Implication for Qt:

- Full placeholder parity can be deferred, but the Qt default name should include stable project/object and plate identity and `.gcode`.

## Current Qt Implementation

### `SliceService`

Relevant files:

- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`

Current strengths:

- Generates real G-code through `Print::export_gcode(...)`.
- Stores active and per-plate output paths.
- Blocks export availability through `EditorViewModel::canExportGCode()` when there is no valid current slice.

Gaps:

- `exportGCodeToPath()` accepts empty/invalid targets.
- It removes an existing target before validating source/target relation.
- It does not reject source/target same path.
- It does not write via a temporary sibling.
- It does not verify final output exists and is non-empty.
- It does not expose current-plate default export filename or all-plate export.
- It does not have explicit exporting state/progress beyond a final status label.

### `EditorViewModel`

Relevant files:

- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`

Current strengths:

- `canExportGCode()` is tied to valid current-plate result, non-empty output path, and not slicing.
- `exportActionHint()` explains missing/stale/slicing states.
- `requestExportGCode()` normalizes file URLs and delegates to `SliceService`.

Gaps:

- No invokable for default export filename.
- No invokable for exporting all printable plates.
- `requestExportGCode("")` is used as a blocked attempt reason path; service should make empty target safe, but QML should not rely on empty target for normal export.

### QML Entry Points

Relevant files:

- `src/qml_gui/pages/PreparePage.qml`
- `src/qml_gui/panels/SliceProgress.qml`
- `src/qml_gui/main.qml`
- `src/qml_gui/BBLTopbar.qml`

Current strengths:

- Topbar and Print menu route to `PreparePage.openExportDialog()`.
- Prepare has a FileDialog for export.

Gaps:

- Export FileDialog does not set `fileMode: FileDialog.SaveFile`.
- SliceProgress export button calls `requestExportGCode(root.outputPath)`, which can be exactly the source path and should never be the normal export target.
- There is no all-plate export destination picker.

### Tests

Relevant file:

- `tests/E2EWorkflowTests.cpp`
- `tests/QmlUiAuditTests.cpp`

Current strengths:

- Existing E2E verifies a basic export copy to a temp path.
- Existing tests block stale/cancelled/missing export state.

Gaps:

- No RED test for same-path protection.
- No RED test for empty target / directory target rejection.
- No RED test for overwrite via temp and final non-empty verification.
- No RED test for per-plate or all-printable-plate export.
- No audit test for SaveFile dialog and SliceProgress avoiding source-path export.

## Implementation Strategy

### Service API

Add or harden APIs:

- `bool exportGCodeToPath(const QString &targetPath)`
- `bool exportPlateGCodeToPath(int plateIndex, const QString &targetPath)`
- `bool exportAllPlateGCodeToDirectory(const QString &directoryPath, const QString &baseName = QString())`
- `QString defaultExportGCodeFileName(int plateIndex = -1) const`

### Safe Copy Algorithm

Recommended helper behavior:

1. Normalize URL/path and reject empty target.
2. Verify source exists, is a file, and is non-empty.
3. Reject if source canonical path equals target canonical/absolute path.
4. Verify/create parent directory.
5. Copy source to a unique sibling temp file.
6. Verify temp file exists and size matches source or is non-empty.
7. Remove/replace final target safely.
8. Rename temp to final target.
9. Verify final target exists and is non-empty.
10. Update progress/status and return success/failure.

### Naming

For v3.4 local workflow:

- Current plate default: `<project-or-object>_plate<N>.gcode` when multi-plate, or `<project-or-object>.gcode` for single-plate.
- All plates: use selected base name plus `_plate<N>` suffix for deterministic outputs.
- Sanitize Windows-invalid filename characters: `< > : " / \ | ? *` and control characters.

### Verification

- RED tests in `E2EWorkflowTests.cpp`:
  - same-path export fails without deleting source;
  - empty target fails;
  - directory target fails;
  - export overwrites an existing different target with non-empty verified output;
  - all-plate export writes deterministic files only for valid printable results.
- RED audit in `QmlUiAuditTests.cpp`:
  - export dialog is `FileDialog.SaveFile`;
  - SliceProgress does not call `requestExportGCode(root.outputPath)` or equivalent;
  - Prepare export dialog uses backend default filename when available.

## Verification Commands

Required:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
git diff --check
```

Focused optional checks:

```powershell
.\build\E2EWorkflowTests.exe -o build\e2e_phase42.txt,txt
.\build\QmlUiAuditTests.exe -o build\qml_phase42.txt,txt
```
