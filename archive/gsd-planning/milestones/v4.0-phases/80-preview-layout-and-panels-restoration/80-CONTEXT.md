# Phase 80 Context: Preview Layout And Panels Restoration

**Phase:** 80
**Milestone:** v4.0 Preview Page UI Restoration
**Date:** 2026-07-06

## Source Truth

- Visual truth: `shotScreen/预览页.png`.
- Behavior truth: `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.*`,
  `GCodeViewer.*`, `GLCanvas3D.*`, and libvgcode role/view metadata.
- Audit input: Phase 79 `79-GAP-MATRIX.md`, especially `PV-TOP`,
  `PV-LEFT`, `PV-VIEWPORT`, `PV-RIGHT-LEGEND`, `PV-RIGHT-GCODE`,
  `PV-STATS`, and `PV-CLEANUP`.

## Current Implementation Baseline

- `PreviewViewModel` already owns G-code parsing, `GCV1` packed payload,
  role visibility masks, 17 view modes, summary/gradient legend data,
  source-line windows, and statistics.
- `PreviewPage.qml` already contains the main regions but exposes visible
  mojibake labels and loose panel sizing.
- `StatsPanel.qml`, `VisibilityFilter.qml`, and `Legend.qml` are structurally
  present but need dense, readable, screenshot-aligned copy and grouping.
- Phase 81 owns the richer layer rail and bottom move/playback behavior; Phase
  80 may keep those regions present but should not expand their behavior.

## Scope

In scope:

1. Restore Preview page region sizing and density for the screenshot-visible
   shell, left sidebar, viewport framing, right analysis stack, and G-code
   source panel.
2. Remove visible mojibake and placeholder copy from touched Preview QML.
3. Preserve all existing `PreviewViewModel` bindings and renderer state
   bindings.
4. Add/extend source-audit tests that fail on visible Preview mojibake or
   missing Phase 80 layout anchors.

Out of scope:

- Changing QRhi/D3D backend policy.
- Replacing Preview parser, role mask, GCV1 payload, or renderer wire format.
- Full layer rail dual-thumb behavior and move playback controls; these belong
  to Phase 81.
- Role color/mode semantic expansion beyond preserving existing bindings; Phase
  82 owns those details.

## Risks

- QML visual edits can accidentally disconnect real ViewModel state.
- `QmlUiAuditTests` single-file QtTest requires rebuild before new assertions
  run; the canonical verifier handles this.
- Full canonical verification can be slow; Phase 80 should use focused audit
  tests while retaining Phase 83 as the final full-build visual gate.
