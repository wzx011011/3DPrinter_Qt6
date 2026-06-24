# Global UI Audit Fix Summary

**Status:** Complete

## Changes

- Added `QmlUiAuditTests` to guard the directly actionable global UI audit fixes.
- Wired `QmlUiAuditTests` into the canonical verification command.
- Removed visible top-level placeholder tabs and phase/version copy from the main shell.
- Routed top-level export/preferences actions to existing dialogs/pages instead of empty handlers.
- Converted the main topbar chrome colors to `Theme.qml` tokens.
- Replaced sidebar mixed English labels and reserved placeholder copy with Chinese runtime copy.
- Raised affected sidebar operational text to theme typography tokens.
- Replaced disabled no-op topbar calibration menu entries with real links into the existing calibration center.

## Verification

- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - Built `OWzxSlicer.exe`
  - Built and ran `QmlUiAuditTests.exe`
  - Confirmed app startup
  - Ran E2E pipeline tests

## Notes

- Large source-truth migrations remain out of scope for this quick task: full AssembleView, ModelMall WebView, and full calibration parity.
- The worktree had pre-existing unrelated dirty changes in several files before this quick task; do not treat those as part of this summary.
