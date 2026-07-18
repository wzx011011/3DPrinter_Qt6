# Phase 166: Dialog Consistency Repair

**Status:** Executed
**Workstream:** Dlg
**Requirements:** Dlg-01, Dlg-02

## Result

- Fixed 8 empty-header dialogs (Dlg-01 BLOCKER): CreatePresetsDialog,
  ExportPresetBundleDialog, NetworkTestDialog, PresetDiffDialog,
  SavePresetDialog, SelectMachineDialog, TroubleshootDialog,
  UnsavedChangesDialog. All used `title:` but CxDialog.qml:14 suppresses
  title="" and exposes `dialogTitle:` — so they were rendering a 44px header
  bar with only the ✕ button. Migrated all 8 to `dialogTitle:`.
- SavePresetDialog was EN-source; swept to ZH (7 strings) per CW-01 policy.

## Verification
- QmlUiAuditTests 124/124 PASS
