# Phase 79: Preview Source-Truth Gap Audit - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Autonomous smart-discuss defaults

<domain>
## Phase Boundary

Phase 79 is a read-only audit phase for the v4.0 Preview page restoration
milestone. It maps the screenshot-visible Preview page regions to:

- the target Preview screenshot,
- the current Qt runtime/source evidence,
- OrcaSlicer Preview and G-code source-truth files,
- current Qt/QML targets,
- downstream phase ownership and verification method.

This phase does not modify product UI source code. It produces the canonical
v4.0 Preview gap matrix that Phase 80, Phase 81, Phase 82, and Phase 83 must
execute against.

</domain>

<decisions>
## Implementation Decisions

### Source And Visual Truth

- Use `shotScreen/预览页.png` as the visual/layout target for v4.0.
- Treat the currently running `build/OWzxSlicer.exe` process as current runtime
  evidence only for app availability; do not claim pixel parity without a
  captured Preview screenshot in Phase 83.
- Use OrcaSlicer source as behavior truth, especially:
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.*`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.*`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.*`
  - `third_party/OrcaSlicer/src/slic3r/GUI/IMSlider.*`
  - `third_party/OrcaSlicer/src/libvgcode/include/Types.hpp`
  - `third_party/OrcaSlicer/src/libvgcode/include/Viewer.hpp`
  - `third_party/OrcaSlicer/src/libvgcode/src/Settings.hpp`
  - `third_party/OrcaSlicer/src/libslic3r/GCode/GCodeProcessor.*`

### Audit Granularity

- Use these Preview region IDs for v4.0: `PV-TOP`, `PV-LEFT`, `PV-VIEWPORT`,
  `PV-RIGHT-LEGEND`, `PV-RIGHT-GCODE`, `PV-LAYER-RAIL`, `PV-MOVE-BAR`,
  `PV-COLOR-MODES`, `PV-STATS`, `PV-INTERACTION`, and `PV-CLEANUP`.
- For each region record target observation, current evidence, Qt target files,
  upstream source, modify-vs-replace decision, severity, owning v4.0 phase,
  requirement mapping, and verification.
- Treat a control as incomplete if it is visible but visually off-target,
  exposes mojibake/raw/internal labels, is dead/no-op, or lacks an upstream
  source map.

### Downstream Ownership

- Phase 80 owns screenshot-visible layout and panel restoration: left sidebar,
  viewport framing, right legend/statistics/G-code panels, vertical layer rail,
  bottom move bar, and placeholder/mojibake cleanup in Preview QML.
- Phase 81 owns layer range/current-layer controls, move stepping/playback,
  keyboard shortcuts, and camera orbit/pan/zoom/fit stability.
- Phase 82 owns G-code role colors, role visibility, color modes, legend
  semantics, and Prepare -> slice -> Preview payload preservation.
- Phase 83 owns stale path cleanup, automated audits/tests, canonical build,
  app launch, and final visual evidence against `shotScreen/预览页.png`.

### the agent's Discretion

- The audit may classify visual-only issues as High when they materially block
  screenshot parity, even if the underlying parser or renderer already works.
- The audit may preserve current real backend paths when behavior is source-
  truth-correct, even if the visible QML shell must be replaced.
- The audit may defer D3D12/Vulkan backend promotion because v4.0 targets the
  verified D3D11/auto path only.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- `src/qml_gui/pages/PreviewPage.qml` owns the current Preview layout, left
  panel reuse, center `GLViewport`, right inspection panel, vertical layer
  rail, and bottom move slider.
- `src/qml_gui/components/StatsPanel.qml` owns print-time/material/move
  statistics and visibility toggles.
- `src/qml_gui/components/Legend.qml` owns discrete/gradient/extruder legend
  rendering.
- `src/qml_gui/components/VisibilityFilter.qml` owns per-role visibility rows.
- `src/qml_gui/components/MoveSlider.qml` owns playback, move position, tool
  change band, current time, and hover-time display.
- `src/qml_gui/components/LayerSlider.qml` contains a richer dual-thumb layer
  range control and tick-code menus, but the current `PreviewPage.qml` uses a
  simpler vertical `Slider` instead.
- `src/core/viewmodels/PreviewViewModel.*` owns G-code parsing, GCV1 packing,
  view modes, role colors, role visibility, statistics, marker data, and
  source-line windows.
- `src/qml_gui/Renderer/RhiViewport*` owns the default QRhi viewport path,
  Preview camera fit, GCV1 segment upload, draw range filtering, and role mask
  filtering.

### Established Patterns

- QML is presentation and wiring only; Preview behavior and source-truth mapping
  belong in C++ viewmodels/services or renderer classes.
- Preview data uses the `GCV1` binary payload from `PreviewViewModel` to
  `RhiViewport`; QML must not parse G-code or mutate renderer payloads.
- Role visibility is render-side filtering through a dense 20-bool mask; it
  must not repack `gcodePreviewData`.
- Current automated coverage already protects many Phase 55 semantics:
  `PreviewParserTests`, `ViewModelSmokeTests`, `QmlUiAuditTests`, and
  `E2EWorkflowTests`.
- Documentation-only audit phases use source reads, `git diff --check`, and the
  encoding guard. The full canonical build is reserved for final verification
  unless source code changes require it earlier.

### Integration Points

- `PreviewPage.qml` binds `previewData`, `layerMin`, `layerMax`, `moveEnd`,
  `roleVisibility`, `showBed`, `showMarker`, `gcodeViewMode`, and tool marker
  coordinates into `GLViewport`.
- `PreviewViewModel::setLayerRange`, `jumpToLayer`, `moveLayerRange`,
  `setCurrentMove`, `togglePlayPause`, and `setViewModeIndex` are the primary
  QML-facing interaction APIs.
- `PreviewViewModel::viewModes()` exposes the 17 upstream view modes.
- `PreviewViewModel::roleVisibilityMask()` exposes the renderer-facing role
  mask; `roleVisibilities()` exposes the UI row list.
- `RhiViewport::fitPreviewCameraToData()` is the current fix for the historical
  Preview orbit-disappears bug.
- `tests/fixtures/orca_sample.gcode` is the current small fixture for Preview
  parser and viewmodel tests.

</code_context>

<specifics>
## Specific Ideas

The target screenshot shows these major regions:

- a top shell with Preview selected and export/slice controls on the top-right;
- a Prepare-like left parameter sidebar still visible while in Preview;
- a large grey 3D/G-code viewport with a bed-centered model/toolpath;
- a compact right legend/statistics table with role colors and visibility icons;
- a G-code text window under the right legend/statistics panel;
- a far-right vertical layer slider with numeric top/bottom labels;
- a bottom move/playback rail spanning the viewport;
- no visible mojibake, raw internal labels, dead controls, or placeholder copy.

The current source already contains real Preview parser/rendering behavior, but
the visible QML shell still needs screenshot-level restoration and label cleanup.

</specifics>

<deferred>
## Deferred Ideas

- D3D12 and Vulkan backend promotion remain future backend work.
- Device send/upload/cloud print and Monitor job lifecycle remain future
  milestones.
- Parameter settings dialog restoration beyond Preview-visible dependencies is
  out of v4.0 scope.
- Full AssembleView restoration is out of v4.0 scope.

</deferred>
