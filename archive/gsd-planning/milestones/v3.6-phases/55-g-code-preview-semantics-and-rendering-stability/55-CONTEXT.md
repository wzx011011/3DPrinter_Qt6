# Phase 55: G-code Preview Semantics and Rendering Stability - Context

**Gathered:** 2026-07-02
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 55 completes the G-code preview *behavior* behind the Preview UI restored in Phase 54 and locks it against renderer regressions. It closes `GCODE-01..05` for the v3.6 local workflow:

- Real G-code segment/layer metadata path from slicing/export into Preview with no placeholder dependency (GCODE-01).
- Color modes and per-role line-type visibility filters for travel, perimeter, infill, support, skirt/brim, wipe tower, and other upstream-equivalent move types (GCODE-02).
- Coherent layer/move filtering that updates GPU draw ranges, legend values, and G-code text / current-line state together (GCODE-03).
- D3D11 QRhi as the normal Windows path, with no regression to `SoftwareViewport` unless explicitly classified as fallback (GCODE-04).
- Regression coverage for camera drag, layer drag, move drag, page switch, reslice, and export (GCODE-05).

This phase does **not** change slicing algorithms, promote D3D12/Vulkan to default, add device/cloud preview, or rebuild the Preview layout (Phase 54 owns layout). It consumes the Phase 40 data semantics and Phase 41 interaction-stability foundations and raises them to source-truth completion inside the restored UI.

</domain>

<decisions>

## Implementation Decisions

### Line-type / per-role visibility filters (GCODE-02)

- Add the **full upstream-aligned per-role visibility set** mapped to `StoredSegment` roles: Travel, Wipe, Seam, Perimeter, External Perimeter, Infill, Solid Infill, Top Solid Infill, Support, Bridge (and others present in upstream `GCodeViewer` move-type visibility list). Audit the upstream `EMoveType` / role list first, then expose what maps to parsed data.
- House the filter UI in a collapsible **"Visible" group inside the right legend/statistics panel**, matching upstream `GCodeViewer`'s right-side visibility list and the restored Phase 54 right panel. (Not left panel; not a top-bar popover.)
- Filtering is **render-side**: a visibility toggle flip updates draw filtering over the already-uploaded segment buffer and calls `update()` only. It must **not** repack `gcodePreviewData`. Repacking happens only on color-mode change, payload change, or resource rebuild. This preserves the Phase 41 interaction-stability guarantee.
- Default visibility matches upstream: travel and wipe hidden after first view; extrusion roles visible. (Not all-on; not all-off-except-extrusion.)

### Color-mode completeness & legend coherence (GCODE-02 + GCODE-03)

- The 13 existing view modes are the candidate complete set. **Audit against upstream `GCodeViewer::EViewType`** and close any gap (e.g. custom-gcode / extra modes) rather than redesigning the list.
- Legend scope under slider filtering is **global** (full slice): min/max/colors reflect the whole toolpath; the per-move current value follows the cursor via the tool-position tooltip. Legend must **not** recompute on every slider drag.
- Gradient legend min/max computed per `StoredSegment` field across all segments, **recomputed only on recolor (mode change)**, not on slider/toggle interaction.
- Keep the existing `stealthMode` property; expose a two-segment Normal/Stealth estimate control near the view-mode combo (upstream PrintEstimatedStatistics Normal/Stealth toggle).

### Real-data-path & no-placeholder verification (GCODE-01)

- Prove no-placeholder via a **RED test on the live local slice path**: run a deterministic slice over a fixture, assert `gcodePreviewData` non-empty, `layerCount > 0`, `moveCount > 0`, and that no segment carries placeholder/demo/sample marker bytes; fail if any demo/sample path is hit. Source-audit grep is supporting, not sufficient.
- Active-result sync must rebuild on `sliceFinished` **and** `resultChanged`; and clear on `sliceFailed`, `sliceResultCleared`, plate-switch-to-invalid, and import invalidation. Each trigger gets a test.
- Stale-preview reset clears **all** Preview state: `gcodePreviewData`, `segments_`, layer/move counts, legend, stats, tool-position, G-code text window, and tick marks — not just stop playback.
- Test fixture source: commit a small realistic Orca-style `.gcode` fixture (extrusion + travel + fan/temp/accel/role comments) for parser/view-mode tests; the GCODE-01 no-placeholder assertion uses the **live** slice path (that is the point of the requirement).

### Interaction regression & renderer stability tests (GCODE-04 + GCODE-05)

- D3D11 no-regression: source-audit that `PreviewPage.qml` never references `SoftwareViewport`, that `main_qml.cpp` registers `RhiViewport` as `GLViewport` on the default path (SoftwareViewport only as init fallback), plus a startup-policy test. No headless GPU capture.
- Drag/page-switch/reslice/export regression: deterministic source/audit tests extending Phase 41's `QmlUiAuditTests`, centered on the **GCV1 payload-survives-interaction invariant** — range/camera/toggle/page-switch setters call `update()` only and never mutate `m_previewData` / `gcodePreviewData_`.
- Reslice invalidation assertion: after a settings change that invalidates, old `gcodePreviewData` is cleared and rebuilt from the new result; assert payload bytes change and layer/move counts recompute.
- Export-stability assertion: the Preview→export path (Phase 42) must not touch the live Preview payload; focused test that exporting while Preview is visible leaves `gcodePreviewData` intact.

### Claude's Discretion

- Exact upstream `EMoveType` → `StoredSegment` role mapping when a 1:1 mapping is ambiguous.
- Internal helper naming for render-side visibility filtering (e.g. a role-mask bitfield on `PackedSegment` / `GcvPackedSegment`).
- How the right-panel "Visible" group is laid out (checkbox list density, scroll behavior) as long as it matches the screenshot and upstream semantics.

</decisions>

<code_context>

## Existing Code Insights

### Reusable Assets

- `src/core/viewmodels/PreviewViewModel.{h,cpp}` already exposes: 13 view modes, `viewModeIndex`, `stealthMode`, `showTravelMoves`/`showBed`/`showMarker`, layer/move range, `gcodePreviewData` (GCV1), legend items + gradient fields, G-code text window (`gcodeLines`/`currentGcodeLine`), tool-position data, per-role times, per-layer times/Z, tool-change bands, tick marks, and `StoredSegment` with per-move fields (feedrate, fan, temp, width, height, layer_time, acceleration, volumetric_rate, extruder_id, layer, move, isTravel).
- `syncPreviewWithActiveResult()` + `rebuildFromGCode()` + `recolorAndPackSegments()` are the data lifecycle entry points; `resetPreviewState()` is the clear path.
- `src/core/services/SliceService.{h,cpp}` emits `sliceFinished`, `resultChanged`, `sliceResultCleared`, `sliceFailed` — the Preview lifecycle signals.
- `src/qml_gui/Renderer/RhiViewport.{h,cpp}` + `RhiViewportRenderer.{h,cpp}` consume GCV1, own draw-range filtering, and call `update()` for interaction setters without mutating `m_previewData`.
- `src/qml_gui/pages/PreviewPage.qml` already binds `previewData`, `layerMin/Max`, `moveEnd`, `gcodeViewMode`, `showTravelMoves` into `GLViewport`; never references `SoftwareViewport`.
- `tests/QmlUiAuditTests.cpp` already guards the RHI normal path, draw spans, camera-fit hook, and no-`SoftwareViewport`-in-`PreviewPage.qml`. Extend this for GCODE-04/05.
- `tests/E2EWorkflowTests.cpp` already has parser regression tests; extend for GCODE-01 no-placeholder.

### Established Patterns

- CPU-prebaked segment colors in `PreviewViewModel`; renderer consumes prebaked color (no renderer-side recolor). Render-side filtering must follow the same contract.
- Single `stateChanged()` NOTIFY for bulk Preview refresh; `tickMarksChanged()` for tick-specific.
- Source-audit tests preferred over headless GPU capture (Phase 41 precedent).

### Integration Points

- Per-role visibility: add a role/move-type field on `StoredSegment` and a packed equivalent on `GcvPackedSegment` so the renderer can mask without reparsing. `PreviewViewModel` owns the visibility-mask state and exposes per-role toggles.
- "Visible" group UI: extend the right panel in `PreviewPage.qml` (and/or a small `VisibilityFilter.qml` component) binding to new `PreviewViewModel` toggle properties.
- No-placeholder test hooks into the same slice fixture path used by `E2EWorkflowTests`.

</code_context>

<specifics>

## Specific Ideas

- Mirror upstream `GCodeViewer` right-side "Visible" list ordering and defaults exactly, since screenshots are visual truth.
- Keep the "Line Type" color mode (feature/role coloring) distinct from the new per-role *visibility* filters — they are independent axes upstream.
- The reslice-invalidates-Preview and export-doesn't-disturb-Preview assertions directly defend the user-reported disappearing-preview class of bug.

</specifics>

<deferred>

## Deferred Ideas

- D3D12 crash root-cause and Vulkan default promotion remain separate backend work (D3D12 crash is an opportunistic deferred item).
- Pixel-perfect visual regression capture for G-code preview remains Phase 58 manual UAT.
- Full upstream Preview rendering parity beyond the v3.6 local workflow (sophisticated marker visuals, shell overlays) continues after the main workflow is stable.
- Device/cloud Preview and send workflows remain outside v3.6.

</deferred>
