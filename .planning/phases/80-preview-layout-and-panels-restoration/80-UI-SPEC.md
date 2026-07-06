# Phase 80 UI Contract

**Surface:** Preview page layout and inspection panels
**Target:** `shotScreen/预览页.png`

## Required Layout

| Region | Contract |
|---|---|
| Left sidebar | Reuse the restored Prepare sidebar at fixed 392 px width, with no Preview-specific duplicate logic. |
| Viewport | Keep the viewport dominant, grey, and framed as a workbench. Empty state is compact and only appears without preview data. |
| Right analysis stack | Fixed 300 px dense panel containing statistics, visibility roles, legend, and source-line panel. |
| G-code source panel | Monospace rows under the analysis stack, bound to `previewVm.gcodeLines` and `currentGcodeLine`. |
| Layer rail | Keep the far-right rail present and stable; deeper layer UX belongs to Phase 81. |
| Bottom move bar | Keep the move bar present and bound; deeper playback UX belongs to Phase 81. |

## Copy And Control Rules

- No visible mojibake remains in `PreviewPage.qml`, `StatsPanel.qml`,
  `VisibilityFilter.qml`, or `Legend.qml`.
- Existing user-visible strings use `qsTr(...)`.
- Every visible toggle continues to call a `PreviewViewModel` method, not a
  QML-only state variable.
- Panel rows use compact heights and elide long text.
- Right panel collapse remains an actual user action.

## Verification

- RED/GREEN audit: `QmlUiAuditTests::previewLayoutRestoresScreenshotRegionsAndGcodePanel`.
- Source audit: confirm `PreviewPage.qml` still binds `GLViewport`,
  `gcodePreviewData`, `roleVisibilityMask`, `gcodeLines`, `StatsPanel`,
  `VisibilityFilter`, and `Legend`.
- Encoding guard and `git diff --check` must pass before commit.
