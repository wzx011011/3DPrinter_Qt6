# Phase 235 Summary — Preset System Completion

**Completed:** 2026-08-15
**Requirements:** PSET2-01..08 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (all 8 suites,
app launches). Iterations: (1) `slots` is a Qt keyword macro — local renamed
slotCount/slotList (C2513/C2059); (2) legacy Phase 85 shell-contract audit
still required the five empty tabs PSET2-08 removed — contract updated to
assert KEPT labels + absence of removed tabs; (3) persistence made the legacy
bundle round-trip test auto-load the exported preset into a same-identity
target — target now gets its own wiped ScopedApplicationIdentity; (4) one
stale-object ABI crash on ViewportContextMenuTests after the Preset/Config
header changes — purged test objects, green; (5) two LNK1104 file locks from
a lingering OWzxSlicer.exe — killed process / deleted exe, green.

## Changes (17 files)

1. **PSET2-01 persistence**: user presets persist to
   `AppDataLocation/user/presets/{printer,filament,process}` in the upstream
   JSON shape (version/name/from/is_custom + type/inherits + values, aligned
   with Preset.cpp:498-536 / Config.cpp:1390-1433). createCustomPreset /
   savePresetValues / deletePreset / renamePreset / importBundle all write
   through; ctor `loadUserPresets()` scans and lists them before system
   presets. Test injection: `setUserPresetDir(QString)` (rescans on change).
2. **PSET2-02**: CreatePresetsDialog scope mapping fixed (UI
   printer/filament/process → PrinterCat/FilamentCat/PrintCat) and inherits
   honored via new `createCustomPreset(cat, name, values, inherits)`
   (nonexistent parent rejected; resolveInheritance reused).
3. **PSET2-03**: `unsavedDialogActive` property + begin/endUnsavedDialog()
   gate — only the first of the three SettingsDialog listeners opens the
   modal, released on accept/reject. UnsavedChangesDialog gained per-row
   checkboxes (default all, checkedKeys) + a Transfer button (switch-*
   pending actions only): `transferPendingChanges(keys)` merges the checked
   keys onto the target preset without saving the source, then executes the
   pending switch. Fixed in passing: applyPendingAction never handled
   `switch-filament-preset-<n>` (Phase 232 slot switches were silently
   dropped behind the guard).
4. **PSET2-04**: importBundleIni printer↔print swap fixed; exportBundleIni
   rewritten as per-preset upstream-JSON tree + index.json manifest (upstream
   uses zip; zip deferred to avoid a new link dependency — documented);
   import reads manifest / category subdirs / root JSON / legacy .ini
   (machine/print aliases); ExportPresetBundleDialog uses FolderDialog.
5. **PSET2-05**: PresetListModel gained presetSection/presetIncompatible
   roles + refreshFromService(service, printer) overload; ConfigViewModel
   exposes decorated{Printer,Filament,Print}PresetNames ("— 用户预设 —" /
   "— 系统预设 —" separators + " (不兼容)" suffix, current selection
   undecorated) + plainPresetName() reverse mapping (request* entries strip
   defensively); CxComboBox renders unselectable separators and disables
   incompatible entries; LeftSidebar + SettingsDialog combos use the
   decorated lists.
6. **PSET2-06**: `setExtruderCount(int)` resizes the slot vector
   (BackendContext wired to editorVm.stateChanged); projectPresetConfigOverlay()
   emits printer/filament/print preset ids + `filament_presets` (";"-joined,
   slot 0 = global); both save paths inject the overlay;
   ProjectServiceMock::saveProjectAs stores it as a ConfigOptionString;
   applyProjectConfig restores it.
7. **PSET2-07**: LeftSidebar preset rows gained a ⋮ menu (rename/delete) with
   an inline rename dialog, delete confirm (in-use warning), read-only
   blockers via canDeletePreset/presetActionBlocker; the dead
   ConfigViewModel::deletePreset early-reject branch removed (default
   fallback now effective, slot references reclaimed); isPresetInUse().
8. **PSET2-08**: the five permanently-empty tabs removed from
   SettingsDialog (printer 材料/挤出机, filament 参数覆盖/依赖 — kMachineKeys/
   kFilamentKeys never map options to those pages; multi-nozzle scope); the
   false comment about per-row reset controls replaced with the real state.

## Tests

ViewModelSmokeTests: `userPresetPersistsAcrossRestart`,
`createPresetHonorsScopeAndInherits`, `bundleImportCategoriesRoundTrip`,
`configTransferPendingChangesAndDialogGate` (single-modal gate),
`configDeleteCurrentPresetFallsBackToDefault`,
`filamentSlotVectorResizesAndPersistsWithProject`; QmlUiAudit:
`presetSystemCompletionSourceAudit`. Legacy
`presetServiceExportsAndImportsUserBundleWithMetadata` updated for
persistence semantics; stale `v50PresetIniAndCreateDialogWired` format
assertions updated to the new tree export.
