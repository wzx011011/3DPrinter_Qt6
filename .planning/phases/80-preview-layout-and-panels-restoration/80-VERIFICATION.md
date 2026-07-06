# Phase 80 Verification

**Phase:** 80 - Preview Layout And Panels Restoration
**Date:** 2026-07-06

## Checks Run

| Check | Result | Notes |
|---|---|---|
| `QmlUiAuditTests.exe previewLayoutRestoresScreenshotRegionsAndGcodePanel` | Pass | RED first failed on missing Phase 80 layout token, then passed after the QML restoration. |
| `QmlUiAuditTests.exe` | Pass | Full QML audit passed: 54 passed, 0 failed. |
| `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py ...` | Pass | Checked touched Preview QML, test file, and Phase 80 planning docs. |
| `git diff --check` | Pass | Only Git line-ending warnings were reported. |
| `qmllint.exe` on touched QML | Partial | `StatsPanel`, `VisibilityFilter`, and `Legend` no longer report missing-property warnings. `PreviewPage.qml` still reports expected command-line import warnings because `OWzxGL.GLViewport` is registered by the app runtime, not available to standalone `qmllint`. |
| `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | Pass | Configure, `OWzxSlicer.exe`, test target builds, PrepareScene, PartPlate, ViewModel, full QML UI audit, PreviewParser, app smoke launch, and E2E pipeline tests passed. |
| `Start-Process .\build\OWzxSlicer.exe` | Pass | App launched from `build/`, window title `OWzx Slicer`, process responding. |

## Coverage

- `PVLAYOUT-01`: left/right/rail/move-bar target dimensions and region anchors
  are present in `PreviewPage.qml`.
- `PVLAYOUT-02`: statistics, visibility roles, legend, and G-code source panels
  remain bound to `PreviewViewModel`.
- `PVLAYOUT-03`: touched Preview QML is covered by mojibake/no-placeholder
  source audit.

## Remaining Gates

- Runtime screenshot comparison remains a milestone gate for Phase 83.
- Phase 81 must replace the simple vertical layer rail with the source-truth
  layer-range/current-layer control.
