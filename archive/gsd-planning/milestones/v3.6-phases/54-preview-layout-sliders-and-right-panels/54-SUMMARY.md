---
status: complete
phase: 54
completed: 2026-07-01
---

# Phase 54 Summary - Preview Layout, Sliders, and Right Panels

## Outcome

Phase 54 restores the Preview page shell around the existing QRhi/D3D11 G-code
preview path. The page now follows the supplied OrcaSlicer screenshot at the
major-region level: left summary/sidebar, center G-code viewport, far-right
vertical layer slider, bottom move slider, and right legend/statistics/G-code
inspection panels.

This phase does not claim final G-code semantic parity. Color-mode details,
line-type filtering depth, renderer stability edge cases, and full interaction
regression coverage continue in Phase 55.

## Implemented

- Rebuilt `PreviewPage.qml` with screenshot-aligned regions for the left
  summary panel, center viewport, vertical layer rail, bottom move slider, and
  right inspection panels.
- Preserved the high-performance Qt renderer path by keeping Preview bound to
  `GLViewport`/QRhi data and adding audit coverage that rejects
  `SoftwareViewport` in the restored Preview path.
- Added side-panel collapse controls for the left and right Preview panels with
  stable viewport sizing and no nested page-card layout.
- Reworked `StatsPanel.qml`, `Legend.qml`, `MoveSlider.qml`, and
  `ToolPositionTooltip.qml` so the visible Preview controls and labels are
  coherent and bound through `PreviewViewModel` invokable setters.
- Extended `PreviewViewModel` with Preview-ready state, status text, layer/move
  labels, plate summary, warning summary, and a bounded G-code source-line
  window synchronized to the current move.
- Extended E2E coverage so parsed G-code exposes the text window and the
  current highlighted source line follows move-slider changes.
- Added QML UI audit coverage for the required Preview layout regions, renderer
  bindings, panel IDs, and absence of file IO or software-render fallback in
  `PreviewPage.qml`.

## Requirement Mapping

| Requirement | Status | Evidence |
|---|---|---|
| PREVLAY-01 | Complete | `PreviewPage.qml` now contains the required left, center, vertical layer, bottom move, and right inspection regions. |
| PREVLAY-02 | Complete | Layer and move sliders call C++ viewmodel APIs without mutating renderer payloads; E2E verifies move-linked G-code text updates. |
| PREVLAY-03 | Complete | Preview remains on the existing `GLViewport` camera/QRhi path; QML audit preserves rotate/pan/zoom viewport binding structure. |
| PREVLAY-04 | Complete | `PreviewViewModel` exposes plate summary, layer/move labels, print-time/filament-backed stats, warning summary, and G-code line data. |
| PREVLAY-05 | Complete | Preview side panels can collapse while preserving the center viewport and avoiding screenshot-region overlap. |

## Deferred

- Full G-code semantic parity, richer line-type filters, and advanced color
  modes are Phase 55.
- Pixel-level screenshot parity and manual Preview visual UAT remain Phase 58.
- Historical mojibake comments outside the touched Preview files remain for the
  cleanup phase unless they block current work.
