# Phase 81 Summary: Preview Layer Move And Playback Controls

**Status:** Implementation complete pending final commit
**Requirements:** `PVCTRL-01`, `PVCTRL-02`, `PVCTRL-03`

## Completed

- Added a compact `PreviewLayerRail.qml` for the far-right Preview rail:
  - vertical `RangeSlider` for visible layer bounds,
  - layer range movement buttons,
  - first/top layer jump actions,
  - direct calls into `PreviewViewModel::setLayerRange`, `jumpToLayer`, and
    `moveLayerRange`.
- Added `PreviewViewModel::stepCurrentMove(int delta)` and routed keyboard
  left/right plus bottom move-step buttons through it.
- Extended `MoveSlider.qml` with previous/next and larger jump buttons while
  preserving the existing play/pause and drag-to-move path.
- Added `RhiViewport::requestPreviewFit()` and wired a Preview fit control in
  `PreviewPage.qml`.
- Kept Preview data ownership in C++:
  - no QML-side move clamping arithmetic,
  - no QML-side camera fit coordinates,
  - no mutation of `m_previewData` during camera or role interaction.
- Added regression coverage:
  - ViewModel smoke coverage for move stepping and G-code source-window sync,
  - QML source audit coverage for rail/playback/fit wiring,
  - corrected the existing Preview interaction setter audit to isolate exact
    function bodies before scanning for forbidden payload refits.

## Not Changed

- No G-code parser, role color, or color-mode semantics were changed.
- No QRhi backend default or D3D12/Vulkan policy was changed.
- No slicing engine behavior was changed.

## Follow-Up

- Phase 82 owns role color, role visibility, and color-mode rendering parity.
- Phase 83 owns final stale-path cleanup, screenshot evidence, runtime visual
  comparison, and final milestone verifier.
