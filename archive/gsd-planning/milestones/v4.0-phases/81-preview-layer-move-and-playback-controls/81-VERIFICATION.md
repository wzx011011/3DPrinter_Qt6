# Phase 81 Verification

**Phase:** 81 - Preview Layer Move And Playback Controls
**Date:** 2026-07-07

## Checks Run

| Check | Result | Notes |
|---|---|---|
| RED source check for Phase 81 tokens | Pass | Initially failed on missing `stepCurrentMove`, `PreviewLayerRail`, `requestPreviewFit`, and page wiring before implementation. |
| `git diff --check` | Pass | Only Git line-ending warnings were reported. |
| `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py` | Pass | Guard reported no staged-file encoding problems; separate UTF-8/BOM check passed for touched files. |
| `ViewModelSmokeTests.exe stepCurrentMoveClampsAndUpdatesGcodeLineWindow` | Pass | After fixing the test setup to reset current move from the loaded fixture's default last-move position. |
| `QmlUiAuditTests.exe previewLayerMoveControlsAreActionableAndRendererSafe` | Pass | Locks `PreviewLayerRail`, ViewModel stepping, `MoveSlider` playback routing, and `requestPreviewFit`. |
| `qmllint.exe` on touched Preview QML | Partial | No fatal lint errors. Existing `MoveSlider` delegate unqualified-access warnings and standalone `OWzxGL.GLViewport` import warnings remain unchanged/expected. |
| `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | Pass | Configure, `OWzxSlicer.exe`, test targets, PrepareScene, PartPlate, ViewModel, full QML UI audit, PreviewParser, app launch, and E2E pipeline tests passed. |

## Coverage

- `PVCTRL-01`: the new Preview layer rail calls `setLayerRange`,
  `jumpToLayer`, and `moveLayerRange`; renderer bindings still consume
  `currentLayerMin/currentLayerMax`.
- `PVCTRL-02`: move drag, keyboard stepping, and previous/next buttons share
  `PreviewViewModel::stepCurrentMove` or existing ViewModel timer methods.
- `PVCTRL-03`: explicit Preview fit reuses cached `m_previewFitHint`, schedules
  redraw, and does not mutate `m_previewData`.

## Remaining Gates

- Phase 82 must finish role/color-mode rendering parity.
- Phase 83 must record runtime Preview screenshot evidence against the target
  screenshot and perform final cleanup audit.
