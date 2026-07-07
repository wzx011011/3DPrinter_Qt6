# Phase 87 UI Spec

## Contract

The settings dialogs must behave like independent upstream settings windows with dirty-preset guards:

1. Selecting another preset while the current tier is dirty opens the unsaved-changes dialog.
2. Save attempts to save the current preset; built-in/read-only presets open Save As and do not close prematurely.
3. Discard restores the selected preset values and then applies the queued action.
4. Cancel closes only the unsaved dialog and clears the queued action; the settings window and current preset remain unchanged.
5. Closing the settings window while dirty opens the same unsaved dialog.
6. Search and advanced-mode filtering are per dialog and preserve dirty/error indicators in the top bar.

## Visual Stability

- Existing Phase 85 shell layout and Phase 86 option-row layout must remain intact.
- No network/device/cloud UI is introduced.
- Action buttons must remain real `CxIconButton`/`CxButton` controls with working handlers.
- Dirty and compatibility indicators stay visible independent of search or tab selection.

## Verification

- Source audit for signal wiring and dialog actions.
- ViewModel smoke tests for save/discard/cancel pending actions and slice invalidation.
- Canonical verifier after implementation.
