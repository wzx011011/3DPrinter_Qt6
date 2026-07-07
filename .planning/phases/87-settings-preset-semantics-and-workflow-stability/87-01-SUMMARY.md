# Phase 87 Summary

## Completed

Restored settings preset dirty-guard workflow semantics.

## Changes

- `SettingsDialog.qml`
  - Added `openUnsavedChangesGuard(closeOnResolve)`.
  - Added `closeAfterUnsavedResolution` to distinguish close-window flows from preset-switch flows.
  - Connected `ConfigViewModel::pendingUnsavedChangesRequested` to `UnsavedChangesDialog`.
  - Wired Cancel/Reject to `requestCancelPendingChanges`.
  - Kept Discard from closing the settings window during preset-switch flows.
  - Preserved read-only Save As handling.

- `QmlUiAuditTests.cpp`
  - Added Phase 87 source audit for pending dirty guard wiring.
  - Updated read-only save audit to expect conditional close behavior.

- `ViewModelSmokeTests.cpp`
  - Added discard-pending transition regression coverage.

## Source Truth Alignment

- Upstream `Tab::may_discard_current_dirty_preset` blocks dirty preset changes until the user chooses Save, Discard, Transfer, or Cancel.
- Qt6 now maps that behavior to Save, Discard, and Cancel using existing `ConfigViewModel` pending actions.
- Transfer is not implemented in this phase because the current Qt6 preset model does not expose per-option transfer selection; it remains outside the immediate v4.1 acceptance criteria.

## Verification

See `87-VERIFICATION.md`.

## Next Phase

Phase 88: Settings Verification And Cleanup.
