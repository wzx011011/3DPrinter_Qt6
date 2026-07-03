# Phase 54 UI Spec - Preview Page

## Screen Contract

The Preview page is an operational analysis surface, not a marketing or dashboard page. The first screen must be the usable preview workflow:

- Left: compact plate summary and layer controls.
- Center: full-height G-code viewport with persistent camera interaction.
- Right: collapsible legend/statistics panel above a G-code text panel.
- Far right: vertical layer range slider surface.
- Bottom: horizontal move/playback slider.

## Layout

- Use the same dark workbench chrome as the existing app.
- Keep the center viewport visually dominant. Side panels may collapse to narrow rails.
- Do not place cards inside cards. Panels may be framed once; repeated rows can use subtle row surfaces.
- Keep fixed control dimensions for sliders, buttons, and counters to avoid layout shifts during slicing or playback.
- Text must fit at 1100px minimum width. Use elision for long G-code lines and file paths.

## Visible Regions

| Region | Required Content |
|---|---|
| `PREV-TOP` | Preview title, view-mode combo, camera preset buttons, estimated/elapsed time chip, layer/move summary |
| `PREV-LEFT` | plate thumbnail/summary, current layer range, move count, print time, filament usage, warning/status line |
| `PREV-VIEWPORT` | registered `GLViewport` in `CanvasPreview` mode, marker tooltip, viewport status overlay only when no Preview data exists |
| `PREV-VSLIDER` | vertical layer-slider surface using `LayerSlider.qml` and current range labels |
| `PREV-MSLIDER` | move playback slider using `MoveSlider.qml` |
| `PREV-RIGHT` | collapsible statistics/visibility controls and G-code text preview |
| `PREV-LEGEND` | collapsible color legend |
| `PREV-STATS` | time, filament, move/layer counts, role/extruder breakdown |
| `PREV-TOOLTIP` | current tool position and move metadata |

## Interaction Contract

- Dragging layer handles calls `PreviewViewModel::setLayerRange()` only.
- Dragging the move slider calls `PreviewViewModel::setCurrentMove()` only.
- View-mode changes call `PreviewViewModel::setViewModeIndex()`.
- Show/hide toggles call `PreviewViewModel` invokable setters.
- Camera orbit/pan/zoom stays inside `RhiViewport`; QML must not reset Preview data in response.
- Side panel collapse affects layout only.

## Copy Contract

- User-facing labels are Chinese via `qsTr()`.
- Code comments are English only.
- No visible placeholders such as "TODO", "reserved", "mock", or "coming soon".
- Empty-state copy may explain the required action: slice or load G-code first.

## Data Contract

- `PreviewViewModel` owns all Preview facts exposed to QML:
  - counts, current layer/move, total time/current time, filament usage, warnings/status, G-code text lines.
- G-code text is exposed as a bounded line window around the active move, not the full file.
- The renderer receives binary `GCV1` data only through `previewData`.

## Verification

- `QmlUiAuditTests` must assert the screenshot-region structure and binding contract.
- ViewModel tests must assert G-code text data is populated from a fixture.
- Manual visual comparison uses `shotScreen/预览页.png` as the target.
