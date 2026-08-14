# Phase 231 Summary — P0 Circuit Break Quick Fixes

**Completed:** 2026-08-15
**Requirements:** CIRC-01, CIRC-02, CIRC-05, CIRC-06, CIRC-07 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (all 8 suites
green, app launches; first run caught 10 additional stale test locks in
QmlUiAuditTests v46-v51 regression slots, fixed and re-verified).

## Changes

1. **CIRC-01 undo dispatch** — `PreparePage.qml` undoFromTopbar/redoFromTopbar
   + the in-page Ctrl+Z/Ctrl+Y shortcuts now route to `editorVm.undo()/redo()`
   (real QUndoStack). Removed the repaint-only `undo()/redo()/clearHistory()`
   traps from `RhiViewport.h` and `SoftwareViewport.h`. `EditorViewModel::undo/
   redo` now also calls `invalidateAllSliceResults()` (upstream invalidates
   slicing on any undo).
2. **CIRC-02 CalibMode** — `CalibrationServiceMock.cpp` pins all 6 modes via
   `kCalibMode*` constexpr from the live `Slic3r::CalibMode` enum (upstream
   deleted Calib_Auto_PA_Line; old ints 5/6/7/8/9 made every tower sweep the
   wrong axis). `towerModelQrcPathForMode` switched to the same symbols. New
   `calibTypeMode(index)` accessor. Decontaminated 3 test locks: QmlUiAudit
   CM-01 block + 10 stale `calibMode = 7/9` regression locks (v46-v51) +
   ViewModelSmokeTests ExpectedCalibRequest tables now assert against the live
   enum and QCOMPARE the dispatched mode.
3. **CIRC-05 export all as one STL/OBJ** — `exportModel` STL/OBJ branches now
   merge every object's instance meshes (volume+instance transforms), mirroring
   `exportObjects::meshForObject`; no longer exports only `objects.front()`.
4. **CIRC-06 mojibake** — `BackendContext.cpp:175` `tr("瀵煎嚭澶辫触")` →
   `tr("Export failed")`; `ConfigViewModel::materialPresetName` category match
   fixed from mojibake `tr("鑰楁潗")` to literal "耗材" (slots now display their
   list entry; full per-slot selection lands in Phase 232).
5. **CIRC-07 CLI --plate** — `CliRunner::runSlice` reworked: `--plate 0`
   (default) now slices ALL printable plates sequentially, exporting one
   plate-labeled G-code per plate via `defaultExportGCodeFileName(plate)`;
   `--plate N` keeps 1-based single-plate slicing; out-of-range N errors.

## Evidence

- Canonical run 2026-08-15 01:48: PrepareScene/PartPlate/ObjectPicking/
  ViewModelSmoke/QmlUiAudit/ViewportContext/PreviewParser/E2E all passed,
  APP_RUNNING_PID=26184, exit 0.
- Regression guard: `calibrationImplementedModesExposeStableRouting` now
  QCOMPAREs each type's dispatched mode against
  `static_cast<int>(Slic3r::CalibMode::…)` — any future enum drift fails tests
  instead of silently sweeping the wrong axis.
