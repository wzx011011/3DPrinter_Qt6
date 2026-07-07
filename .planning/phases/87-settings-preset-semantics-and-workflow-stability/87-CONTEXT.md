# Phase 87 Context: Settings Preset Semantics And Workflow Stability

## Scope

Restore settings preset workflow semantics for the v4.1 settings-dialog milestone.
Network, LAN device, cloud, Monitor, and online workflows are excluded by user direction.

## Requirements

- SETSEM-01: Preset selection, save, save-as, reset option/group/all, discard, cancel, and unsaved-close guard remain mapped to upstream settings semantics.
- SETSEM-02: Search and simple/advanced filtering work per dialog without breaking tab/section navigation or hiding current dirty/error states.
- SETSEM-03: Settings edits invalidate slice state, preserve dirty overrides through project save/load, and keep Prepare/Preview payloads stable across settings dialog interaction.

## Source Truth

- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:6022` blocks preset selection when the current preset is dirty and the unsaved dialog is cancelled.
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:6256` defines dirty-preset switching through `may_discard_current_dirty_preset`.
- `third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp:1003` exposes Discard/Don't save and Save actions.
- `third_party/OrcaSlicer/src/slic3r/GUI/GUI_App.cpp:8014` confirms the app-wide Save/Discard/Cancel dirty guard.

## Existing Qt6 State

- `ConfigViewModel` already has pending unsaved actions: `pendingUnsavedAction`, `pendingUnsavedTarget`, `pendingUnsavedChangesRequested`, `requestSavePendingChanges`, `requestDiscardPendingChanges`, and `requestCancelPendingChanges`.
- `SettingsDialog` opens `UnsavedChangesDialog` on close, but does not listen to `pendingUnsavedChangesRequested`; dirty preset selection can queue a pending action with no visible dialog.
- `SettingsDialog` save-as handling is present through `saveAsRequired`, but the source-audit contract should cover the signal connection.
- Tests already cover several backend basics: preset dirty tracking, reset, read-only save-as, filter indices, and config-change slice invalidation.

## Primary Gap

Dirty preset switching is not fully connected in QML:

1. User edits a setting.
2. User selects another preset.
3. `ConfigViewModel::requestCurrent*Preset` queues a pending action and emits `pendingUnsavedChangesRequested`.
4. `SettingsDialog` ignores the signal, so Save/Discard/Cancel is not shown.
5. A later Cancel does not clear pending state because the dialog is not wired to `requestCancelPendingChanges`.

## Implementation Direction

- Keep business rules in `ConfigViewModel`; QML only wires visible interactions.
- Add tests before behavior changes.
- Keep UI changes inside settings dialogs and tests.
- Preserve Phase 85/86 visual contracts.
