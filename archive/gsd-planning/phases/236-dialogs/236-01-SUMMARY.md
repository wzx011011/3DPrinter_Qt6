# Phase 236 Summary — Dialog Reachability And Completion

**Completed:** 2026-08-15
**Requirements:** DLG-01..04 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (all 8 suites,
app launch, E2E pipeline). Iterations: (1) the first 236 verify hit a
PartPlateTests::arrangeDistributesAcrossPlates failure that bisected clean —
HEAD state passed and a from-scratch rebuild of the full 236 tree also passed
(55/55, `[DBG236] plateCount= 3`), i.e. the known stale-object ABI artifact,
not a code regression; the stash/pop round-trip forced consistent recompiles.
(2) dialogReachabilitySourceAudit needed two fixes: the emitter check matched
`showX()` literally while `showEditGCodeDialog(key, value)` takes parameters
(now matches the definition's opening paren), and the LGPL ban was tightened
from any "LGPL" to the wrong license claim "GNU LGPL" so the accurate
`Qt (GPL/LGPL)` component attribution stays allowed.

## Changes (28 files: 23 modified + 5 new dialogs)

1. **DLG-01 entry points**: File menu gains 导出预设包... (Export Preset
   Bundle → showExportPresetBundleDialog) and a 工具 (Tools) submenu hosting
   the upstream Preferences-hosted tool dialogs (AMS 设置 / 固件升级 /
   限速设置 / 插件管理 / 精简模式) via their BackendContext emitters
   (BBLTopbar.qml); PreferencesPage mirrors the same entries for when it
   mounts in the shell. EditGCode trigger: machine start/end G-code option
   rows expose an edit affordance that calls
   `backend.showEditGCodeDialog(key, value)` (OptionRow.qml:484-515).
2. **DLG-02 save paths**: EditGCodeDialog onGcodeAccepted now persists via
   `ConfigViewModel::setValue(optionKey, text)` (was a no-op log);
   WipeTowerDialog OK persists the flush matrix through new
   `PresetServiceMock::saveFlushVolumes` (QVariantList + QList<QList<double>>
   overloads) writing the upstream `flush_volumes_matrix` coFloats key.
3. **DLG-03 new dialogs + service plumbing**:
   - FileArchiveDialog (zip/3mf multi-select import) backed by
     `ProjectServiceMock::listArchiveEntries/extractArchiveEntry` (miniz
     central-directory read, model-extension filter) and
     `EditorViewModel::importArchiveEntries`; main.qml routes .zip imports
     to it instead of a direct load.
   - ObjColorDialog (OBJ mtl → extruder mapping) backed by
     `ProjectServiceMock::objMtlColors` (Kd diffuse parsing, de-duped) +
     `EditorViewModel::applyPendingObjColors`; objColorMappingRequested
     signal drives it after OBJ loads with an mtl.
   - RecenterDialog (out-of-bed prompt) backed by
     `EditorViewModel::checkObjectsOutsideBed/recenterObjectsOutsideBed`
     (objectsOutsideBed list; upstream Plater outside-bed detection
     semantics); recenterPromptRequested signal from BackendContext.
   - SysInfoDialog (system info dump) backed by
     `BackendContext::systemInfo()` (Qt/OS/GPU/graphics-API via manual
     QSGRendererInterface::GraphicsApi switch — the enum is not Q_ENUM).
   - SingleChoiceDialog (shared generic picker, exempt from the trigger
     audit by design).
   - 3MF generator-version disclosure: `ProjectServiceMock::
     projectVersionInfo` parses the 3MF Application metadata
     (read3mfGeneratorVersion); surfaced as a notification instead of a
     modal (no update server in OWzx) — the upstream
     Newer3mfVersion/MsgDataIncompatible warning family.
4. **DLG-04 AboutDialog license**: LGPL claim replaced with the upstream
   AGPL-3.0 statement + OrcaSlicer/PrusaSlicer/BambuStudio lineage +
   open-source components list (upstream AboutDialog.cpp:148-156 m_entries
   structure, incl. accurate `Qt (GPL/LGPL)` attribution).

## Tests

- QmlUiAuditTests::dialogReachabilitySourceAudit (+209 lines): every
  dialogs/*.qml is qrc-registered, instantiated outside its own file, and
  has ≥1 trigger token; table + files stay in sync; BackendContext
  declares+implements every emitter; AboutDialog AGPL claims; EditGCode
  setValue path; WipeTower flush_volumes_matrix path. 143/143 green.
- ViewModelSmokeTests: wipeTowerSaveFlushVolumesRoundTrip (matrix → service
  → readback), editorCheckObjectsOutsideBedDetectsOutsideObject (placed
  object detected outside, recentered back inside).
