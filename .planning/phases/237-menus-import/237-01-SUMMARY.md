# Phase 237 Summary — Menus, Shortcuts, Drag-Drop, And Import

**Completed:** 2026-08-15
**Requirements:** VIEW-01..06 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (agent run +
independent re-run before commit; all 8 suites, app launch, E2E pipeline).

## Changes (19 files)

1. **VIEW-01 View menu**: the disabled 视图 stub (BBLTopbar.qml:866-889) is
   now the full upstream View menu — 7 camera presets routed through new
   `RhiViewport::selectView(QString)` (upstream direction names,
   GLCanvas3D.cpp:3192-3201; "plate" also zooms to bed), perspective/
   orthographic radio pair, Show G-code Window check (Preview-gated).
   CameraController gained bottom/rear/left presets (Camera.cpp:86-107) and
   a real ortho branch in projMatrix with half-height matched to the
   perspective frustum (upstream MainFrame.cpp:2604-2620 radio pair).
   PreviewPage exposes previewViewportRef; the G-code window toggle binds
   through new PreviewViewModel::showGcodeWindow (QSettings-persisted;
   upstream app_config show_gcode_window).
2. **VIEW-02 Edit/File + shortcuts**: 全部删除 Ctrl+D wired to
   clearWorkspace() (was zero callers) behind a destructive confirm;
   Import Zip Archive (Phase 236 FileArchiveDialog flow); 导入配置...
   (PresetServiceMock::importBundle + result notification, upstream
   MainFrame.cpp:3204-3247; filter is .json — the one format with a real
   importer); 导出切片文件... Ctrl+G (gated on slice result).
   main.qml binds Ctrl+N/Ctrl+Shift+S/Ctrl+A/Esc/Ctrl+P/Ctrl+R/Ctrl+G/
   Ctrl+Shift+G/Ctrl+0..6 to real APIs. Ctrl+P corrected to Preferences
   per upstream truth (KBShortcutsDialog.cpp:197), not print. KBShortcuts
   Dialog: Ctrl+D label fixed to "Delete all" (was wrongly "Duplicate
   selection"; upstream Ctrl+K is Clone) and lists aligned with
   KBShortcutsDialog.cpp:173-215.
3. **VIEW-03 drag & drop**: window-level DropArea in main.qml (PlaterDrop-
   Target port, Plater.cpp:2738-2767) — activates window + switches to
   Prepare; multi-file → `addFilesToCurrentPlate`; single .svg parks on
   the SVG gizmo panel field (upstream picks a surface point via raycast
   which has no drop-time equivalent); .zip → FileArchiveDialog; non-model
   extensions filtered silently.
4. **VIEW-04 unit inference + notices**: `ProjectServiceMock::
   loadedObjectUnitHint` ports the upstream thresholds (0.008 m³ meters /
   8.0 in³ imperial, meters-before-imperial; Model.cpp:763-815 +
   Plater.cpp:4237-4253); `EditorViewModel::applyUnitConversion` routes to
   `ModelObject::convert_units`; zero-volume objects removed with a
   notification (Model.cpp:830-844, epsilon 1e-10); prompt fires from the
   loadFinished hook for model imports only (upstream !is_project_file).
5. **VIEW-05 embedded config**: .3mf/.cxprj project opens route through
   loadProject (LoadModel|LoadConfig → projectConfigLoaded →
   applyProjectConfig) instead of loadFile's LoadModel-only path.
6. **VIEW-06 .gcode.3mf + scale-to-fit**: `exportGcode3mf(plate, dest)`
   writes the plate 3MF (shared `storeProject3mf` writer extracted from
   saveProject — same body, saveProject keeps its bookkeeping) and appends
   `Metadata/plate_N.gcode` (entry name per GCODE_FILE_FORMAT,
   bbs_3mf.hpp:22; upstream Plater.cpp:11499-11573). Note: ships the full
   model entries (upstream SkipModel filters them; Qt6 StoreParams exposes
   no per-plate filter) — a superset with the exact upstream gcode entry.
   `EditorViewModel::scaleSelectionToFitBed` ports Selection::
   scale_to_fit_print_volume (Selection.cpp:1449-1462: +0.02mm guard,
   min-ratio uniform scale, re-center + drop to plate, single undo macro).

## Tests

- QmlUiAuditTests::viewMenuShortcutsAndImportSourceAudit (+163 lines):
  preset wiring, all shortcut sequences present in main.qml, DropArea,
  KBShortcutsDialog Ctrl+D label, stub removal, VIEW-04/05/06 plumbing
  declarations. Suite 144 green.
- ViewModelSmokeTests (+367 lines, suite 141 green):
  editorUnitInferenceDetectsSavedUnits (synthetic 0.1/1.5/20-unit cubes →
  meters/imperial/none + prompt), editorApplyUnitConversionScalesObjectTo-
  Millimeters (×1000 via world bbox), editorScaleSelectionToFitBedShrinks-
  OversizedObject (400mm cube fits bed, on plate), projectServiceExport-
  Gcode3mfProducesArchive (zip readable: Metadata/plate_1.gcode +
  3D/3dmodel.model), loadProjectAppliesEmbeddedProjectConfig (layer_height
  0.42 round-trip).

## Honestly-gated / upstream deltas (documented in code)

- Show Labels / Show Overhang / Show Selected Outline / 3D Navigator /
  Reset Window Layout left OUT (no renderer consumer exists — no fake
  toggles).
- Ortho toggle is per-viewport state; upstream persists it globally
  (use_perspective_camera) — deferred until an app_config surface exists.
- .svg single-drop parks on the gizmo panel field instead of auto-creating
  an object (upstream surface-point pick has no drop-time equivalent).
- Zero-volume removal is a notification (upstream modal) — OWzx dialog
  pattern.
