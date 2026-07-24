# Phase 195 Plan: KBShortcutsDialog Extraction and Grouping

**Requirement:** UI-02
**Goal:** Extract the inline shortcut overview Dialog from `main.qml` into an
independent `dialogs/KBShortcutsDialog.qml`, reorganized as a 5-group view
(Global/Prepare/Toolbar/ObjectsList/Preview) aligned with upstream
`KBShortcutsDialog.cpp`. Reconcile displayed shortcuts with actual bindings.

## Files

- Create: `src/qml_gui/dialogs/KBShortcutsDialog.qml`
- Modify: `src/qml_gui/main.qml` (replace inline Dialog with the new component)
- Modify: `src/qml_gui/qml.qrc` (register the new dialog)

## Steps

- [x] Create `dialogs/KBShortcutsDialog.qml` with a 5-group layout
      (Global / Prepare / Toolbar / ObjectsList / Preview) using a left
      group selector + right content, aligned with upstream
      `KBShortcutsDialog.cpp:167-330`.
- [x] Map the existing 26 shortcut entries into the 5 groups:
      Global (Ctrl+Z/Y/I/O/S/P, Undo/Redo/Import/Open/Save/Prefs),
      Prepare (F/W/E/R, fit/move/rotate/scale), Toolbar (Ctrl+1/3/6/0 views),
      ObjectsList (Ctrl+X/C/V/D/K, Delete/Escape/A), Preview (Space, arrows,
      Home/End, PgUp/Dn, Shift+PgUp/Dn).
- [x] Replace the inline Dialog in `main.qml:116-200` with
      `KBShortcutsDialog { id: shortcutDialog }`.
- [x] Register the dialog in `qml.qrc`.
- [x] Note: F/W/E/R/Space/arrows are handled in C++ keyPressEvent
      (EditorViewModel/GLViewport), not QML Shortcut{}. The dialog documents
      the user-visible shortcuts regardless of binding mechanism; QML
      Shortcut{} bindings (Undo/Save/Redo/Delete/Import/Open/Cut/Copy/Paste/
      Duplicate) are the subset bound at the main.qml level.

## Verify

- [ ] `grep -n "shortcutDialog\|KBShortcutsDialog" src/qml_gui/main.qml` shows
      the new component usage, no inline Dialog block remains.
- [ ] Canonical build exits 0 (pending environment fix).
- [ ] QmlUiAuditTests PASS.
- [ ] Launch liveness confirmed.
