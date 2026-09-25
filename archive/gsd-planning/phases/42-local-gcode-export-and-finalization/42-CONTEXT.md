# Phase 42: Local G-code Export and Finalization - Context

**Gathered:** 2026-06-29
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 42 completes local `.gcode` export semantics for the v3.4 main workflow. It owns current-plate export, all-printable-plate export, target naming, selected path handling, same-path/self-copy protection, output verification, export progress/status, and UI entry points.

It does not implement device upload/cloud printing or printer-host send. It also does not change slicing algorithms. If export requires a fresh valid slice, Phase 42 must use the existing slice result state from Phases 38-41 and block stale/missing results rather than silently copying old output.

</domain>

<decisions>

## Implementation Decisions

### Source-Truth Mapping

- D-42-01: Upstream source truth is `Plater::export_gcode`, `Plater::priv::export_gcode`, `BackgroundSlicingProcess::finalize_gcode`, `BackgroundSlicingProcess::export_gcode`, and `PrintStatistics::finalize_output_path`.
- D-42-02: Qt keeps its simpler `SliceService`-owned generated G-code files, but export behavior must match visible upstream semantics: user chooses a destination, invalid/stale results are blocked, copy/finalization failures are surfaced, and the final file is verified.
- D-42-03: Export current plate and export all printable plates are local-only. Device/removable media upload remains future scope.

### Export State and Safety

- D-42-04: `SliceService::exportGCodeToPath()` must reject empty targets, missing sources, directories-as-target, and source/target same canonical path.
- D-42-05: Export should write through a temporary file and verify the destination exists and is non-empty before reporting success.
- D-42-06: Overwrite is allowed only when the user-selected save path is different from the active generated source path. Existing destination files may be replaced after safe temp-copy verification.
- D-42-07: Export status/progress should be visible through existing `statusLabel`, `progressChanged`, `progressUpdated`, and `stateChanged` signals.

### Naming and UI

- D-42-08: The export dialog must be `FileDialog.SaveFile`, not an open dialog, and should present a useful default `.gcode` filename derived from the project/object/plate context where available.
- D-42-09: SliceProgress's export button must open the export dialog or pass an explicit user target. It must not call `requestExportGCode(outputPath)` because that attempts to export to the source path.
- D-42-10: All-plate export should produce deterministic per-plate names under the selected directory/base path and skip locked/non-printable/missing plates with explicit failure state.

### Verification

- D-42-11: Add RED tests before production changes for same-path protection, missing/empty target handling, verified copy, per-plate export, and QML SaveFile wiring.
- D-42-12: Canonical verification remains the accepted full build/test command: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

</decisions>

<code_context>

## Existing Code Insights

### Current Qt Export Path

- `SliceService::exportGCodeToPath(const QString &targetPath)` currently:
  - fails only if `outputPath_` is empty or source does not exist;
  - removes existing target unconditionally;
  - copies `outputPath_` to `targetPath`;
  - reports success without same-path protection or destination verification beyond `QFile::copy`.
- `EditorViewModel::requestExportGCode()` delegates directly to `SliceService::exportGCodeToPath()`.
- `PreparePage.qml` has an `exportGCodeDlg`, but it does not set `fileMode: FileDialog.SaveFile`.
- `SliceProgress.qml` calls `root.editorVm.requestExportGCode(path)` where `path` defaults to `root.outputPath`, making the button attempt to export to the source file itself.
- `QmlUiAuditTests::prepareReadinessControlsBindBackendAvailability()` currently only checks that export is guarded, not that it uses a save dialog or avoids same-path export.

### Existing Per-Plate Result State

- `SliceService` stores per-plate results in `plateResults_`, with `plateOutputPath()`, `hasPlateResult()`, and `activatePlateResult()`.
- `EditorViewModel::requestSliceAll()` slices unlocked printable plates and Phase 39 verifies skipped locked/non-printable plates.
- There is no current public API for exporting all plate results to deterministic paths.

### Existing Tests

- `E2EWorkflowTests::test_export_gcode_to_path()` verifies a basic copy after a successful slice.
- Phase 38/39 tests verify stale/cancelled/missing slice results block `canExportGCode`.
- No test currently checks same-path protection, empty target, verified output, or all-plate export.

</code_context>

<specifics>

## Specific Ideas

- Add a small `SliceService` helper for safe copy/finalize:
  - normalize source and destination with `QFileInfo::canonicalFilePath()` when possible;
  - create parent directories if allowed or fail clearly;
  - copy to `targetPath + ".tmp"` or a unique sibling temp file;
  - compare sizes or at least verify non-empty final output;
  - rename atomically where possible.
- Add `Q_INVOKABLE QString defaultExportGCodeFileName(int plateIndex = -1) const`.
- Add `Q_INVOKABLE bool exportPlateGCodeToPath(int plateIndex, const QString &targetPath)` and `Q_INVOKABLE bool exportAllPlateGCodeToDirectory(const QString &directoryPath, const QString &baseName = QString())`.
- Keep `requestExportGCode()` for current-plate UI, but make blocked calls update `statusText_` and `stateChanged()`.
- Add QML audit checks that `exportGCodeDlg.fileMode` is `FileDialog.SaveFile` and `SliceProgress.qml` no longer exports to `root.outputPath`.

</specifics>

<deferred>

## Deferred Ideas

- Device send/upload/cloud print remains outside v3.4 local export.
- Removable-media detection and post-processing scripts are not implemented in the Qt app unless already present in generated G-code flow.
- Full upstream filename-template placeholder parity can be expanded later if local default names are deterministic and safe for v3.4.

</deferred>
