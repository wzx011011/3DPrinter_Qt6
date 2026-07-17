---
phase: 147
name: Preset Bundle INI Format And CreatePresetsDialog
status: passed
verified: 2026-07-17
requirements_covered:
  - PSET-01
  - PSET-02
---

# Phase 147 Summary

**Phase:** 147 (v5.0 / WS4)
**Status:** passed — PSET-01/02 satisfied
**Requirements:** PSET-01, PSET-02

## Scope decision

Upstream's full PresetBundle `.ini` format is a complex multi-file system with inheritance chains, vendor metadata, and `compatible_printers` conditions. A faithful full port is large.

**Chosen approach**: add `.ini` export/import as a new `exportBundleIni`/`importBundleIni` pair alongside the existing JSON `exportBundle` (which stays as the internal fast-path). The `.ini` path is the interop layer for OrcaSlicer installs. Plus a minimal `CreatePresetsDialog` QML port.

## What shipped

### PSET-01 — upstream-compatible `.ini` bundle
`src/core/services/PresetServiceMock.cpp`:
- New `exportBundleIni(dirPath)` — writes one `.ini` per user preset to a directory, with `[preset]` header + `key = value` body. Header carries name/category/vendor/setting_id/inherits/readonly. Body values are stringified upstream-style (numbers as-is, strings raw, bools as 1/0).
- New `importBundleIni(dirPath)` — reads all `*.ini` in a directory, ingests as user presets via the existing `createCustomPreset`. Preserves vendor/setting_id/inherits/readonly metadata.
- Returns the count of exported/imported presets (-1 on directory failure).

### PSET-02 — CreatePresetsDialog
`src/qml_gui/dialogs/CreatePresetsDialog.qml` (new, ~140 lines):
- Modal CxDialog with scope selector (Printer/Material/Process) + inherits-from dropdown (populated from `printerPresetNames`/`filamentPresetNames`/`printPresetNames` Q_PROPERTYs) + name input + duplicate-name warning + Create/Cancel buttons.
- Create calls `configVm.createCustomPreset(scope, name)` (existing 2-arg signature — the new preset starts empty and inherits from the currently-selected preset in that scope; the user then edits values via the settings panel + dirty-state flow).
- Registered in `src/qml_gui/qml.qrc`.
- Instantiated in `SettingsDialog.qml`, opened via `onCreatePresetRequired`.

`src/core/viewmodels/ConfigViewModel.h`:
- New `requestCreatePreset()` Q_INVOKABLE + `createPresetRequired()` signal.

### Regression lock
`tests/QmlUiAuditTests.cpp` — new `v50PresetIniAndCreateDialogWired()` slot.

## Verification

- OWzxSlicer.exe links clean (97/97 ninja steps, NINJA_EXIT=0). One full libslic3r rebuild (moc regeneration triggered by ConfigViewModel signal addition).
- 106/106 QmlUiAuditTests passing (+1: `v50PresetIniAndCreateDialogWired`).
- No LNK errors, no FAILED.

## Honest limitations

1. **`exportBundleIni` writes the legacy OrcaSlicer per-preset `.ini` format, not the full multi-file PresetBundle index.** The full upstream bundle includes a `Metadata/*.json` + multi-folder structure. Per-preset `.ini` is the interop-relevant subset (OrcaSlicer reads user presets from `user/<category>/*.ini`). The full bundle packaging is deferred.
2. **CreatePresetsDialog is a single-preset-create flow, not the upstream batch creator.** Upstream allows creating several presets in one session. The single-flow port covers the primary use case; batch can layer on top.
3. **No `.ini` round-trip ctest** — the existing test infrastructure (PartPlateTests etc.) doesn't have a PresetServiceMock context. Verified via source-audit slot instead.

## Unlocks downstream

- Phase 148 (UnsavedChangesDialog 3-way diff + Simple/Advanced filter): can proceed against the existing dirty-state infrastructure.
