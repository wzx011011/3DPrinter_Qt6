# Phase 174: Per-Object Settings Override Dialog

**Status:** Executed
**Workstream:** FEAT
**Requirement:** FEAT-01

## Result

Ship the QML inspector for per-object print/filament parameter overrides.
Mirrors upstream GUI_ObjectSettings.

Backend: ProjectServiceMock already exposes scopedOptionValue/setScopedOptionValue/
scopedOverrideCount/scopedOverriddenKey/resetScopedOptionValue. Phase 174 adds:
- EditorViewModel proxies (Q_INVOKABLE) for all 5 scoped APIs (was signal-only)
- SelectionSettingsDialog.qml (new): CxDialog with 6 common FDM override keys
  (layer_height/fill_density/wall_loops/support_material/nozzle_temperature/
  bed_temperature). Each row: label + value input (numeric via CxTextField,
  bool via CxCheckBox) + reset button. Overridden-keys count footer.
- PreparePage instantiates the dialog + binds onSelectionSettingsRequested
  (the signal was firing from 4+ sites but had no consumer).
- qml.qrc registration.

## Verification
- QmlUiAuditTests 132/132 PASS
- OWzxSlicer link OK
