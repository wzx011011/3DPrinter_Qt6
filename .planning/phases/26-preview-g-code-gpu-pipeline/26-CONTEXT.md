# Phase 26: Preview G-Code GPU Pipeline - Context

**Gathered:** 2026-06-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Add a `CanvasPreview` render branch to `RhiViewportRenderer` that renders G-code preview segments through QRhi using a segment-buffer pipeline with GPU draw-range layer filtering, pre-baked color modes, and visibility toggles. This replaces the legacy `GCodeRenderer` (QQuickFramebufferObject + per-frame CPU filter) when `OWZX_RHI_RENDERER` is enabled.

**Key scout finding:** This is a "follow established pattern" task, NOT "design from scratch." Phase 23-25 already built: the RhiViewport host (with all Preview Q_PROPERTY plumbing), the QRhi buffer/upload/shader conventions, the camera uniform, and even a validated segment shader + 1M-segment benchmark. The QML routing needs ZERO changes (`GLViewport` name maps to RhiViewport under the gate). The substantive new work is: (a) Preview render branch, (b) GCV1 parsing + layer-offset indexing, (c) marker geometry.

**In scope:** Preview render branch in RhiViewportRenderer; GCV1 blob → GPU segment buffer; layer-range draw-range control (no per-frame CPU filter); travel/extrusion visibility toggles; tool-position marker geometry on QRhi.
**Out of scope:** GPU-side color mode computation (deferred — CPU pre-baking is the pattern); GCodeProcessorResult.moves direct consumption (PreviewViewModel re-parses .gcode text today — that refactor is separate); performance benchmarking at 1M/5M (Phase 27); fallback hardening (Phase 28).

</domain>

<decisions>
## Implementation Decisions

### GPU buffer layout — reuse Prepare's Vertex convention (D-26-01)
- **D-26-01:** Use the existing `Vertex { float x,y,z, r,g,b,a }` 28-byte layout (same as Prepare rendering, `RhiViewportRenderer.h:18`) and the existing `m_linePipeline` (Lines topology). Each `PackedSegment` (80-byte GCV1 record) expands to 2 vertices (start xyz + end xyz) sharing RGBA. Axis swap: GCode `y`↔`z` to GL space (mirror `GCodeRenderer.cpp:527-528`). Keep layer/move/extruder_id out of the GPU vertex (they're for filtering, addressed by D-26-02). **Zero new shaders needed** — the existing `segment.vert/.frag` (add CameraBlock uniform + vec3 position) or `rhi_viewport.vert/.frag` already match this layout.

### Layer-range control — GPU draw range via pre-sorted layer-offset index (D-26-02)
- **D-26-02:** Pre-sort segments by `layer` at GCV1-parse time, build a `layer → vertex-offset` index, then use `cb->draw(vertexCount, firstVertex)` with computed offset/length on layerMin/layerMax/moveEnd change — **no buffer re-upload, no per-frame CPU loop**. This is the single biggest Preview perf win (the current GL path rebuilds a filtered vector every frame, `GCodeRenderer.cpp:290-309`) and aligns with Phase 27's 1M/5M target. The render_bench already proves raw draw is fast; the bottleneck today is the CPU filter.
  - Sorting: segments arrive in `layer` order in practice (G-code is layer-sequential), but verify + sort defensively. Build offset index once per GCV1 re-parse.
  - moveEnd (playback): maintains a separate running vertex offset for the playback position; draw from 0 to moveEnd-offset when playing.

### Color mode — CPU pre-baked (existing pattern), NOT GPU-side (D-26-03)
- **D-26-03:** PreviewViewModel already bakes the active color mode's RGB into the GCV1 blob (`recolorAndPackSegments`, `PreviewViewModel.cpp:929-962`) and only re-packs on mode change. The QRhi renderer treats color as opaque RGBA — **no mode logic in the GPU pipeline**. Trade-off: switching color mode re-uploads the whole segment buffer (acceptable — mode switches are infrequent; render_bench shows uploads complete in ms even at 1M segments). GPU-side recolor (richer but riskier: new shaders + palette texture + larger vertex) deferred to a later optimization only if mode-switch latency is measured as a problem.

### Marker geometry — replicate on QRhi (D-26-04)
- **D-26-04:** The tool-position marker (cone + cylinder arrow, `GCodeRenderer.cpp:353-431`) must render on QRhi when `showMarker` is true. Build the marker as static geometry (vertices) uploaded to a small QRhiBuffer, drawn with the same camera-uniform pipeline. Lower priority than the segment pipeline — if time-pressed, ship segment rendering first and defer marker to a sub-plan.

### Claude's Discretion
- Whether to use the existing `segment.vert/.frag` (NDC-only, needs CameraBlock + vec3 upgrade) or just reuse `rhi_viewport.vert/.frag` directly (already has CameraBlock + vec3 + color passthrough — simplest).
- Exact layer-offset index data structure (vector of {layer, vertexOffset, vertexCount} or a simple map).
- Whether marker is in the main plan or a separate sub-plan.
- Travel-move visibility: GPU-side (separate buffer) or pre-filtered into the same buffer with a toggle uniform — recommend same-buffer + draw-range split (extrusion range vs travel range) since segments are already categorized.

</decisions>

<canonical_refs>
## Canonical References

### Phase 23-25 established patterns (MUST follow)
- `src/qml_gui/Renderer/RhiViewportRenderer.h:18` — Vertex layout (28-byte, the convention to reuse)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:370` — ensureBuffer() helper (Static for vertices, Dynamic for uniforms)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:180-215` — pipeline/SRB/vertex-input pattern (camera uniform binding 0, Lines topology pipeline)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:364` — clip-space correction `rhi()->clipSpaceCorrMatrix() * mvp`
- `src/qml_gui/Renderer/RhiViewport.h:21-51` — Preview Q_PROPERTY surface ALREADY EXISTS (previewData/layerMin/Max/moveEnd/gcodeViewMode/showTravelMoves/showMarker/markerXYZ)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:24,32,84` — synchronize() already reads m_previewBytes but discards; render() only handles CanvasView3D (line 84)
- `.planning/phases/2[345]-*/*SUMMARY*.md` — buffer/shader/gate/scene-data conventions

### Current Preview (the code being replaced/equivalent)
- `src/qml_gui/Renderer/GCodeRenderer.cpp:476-548` — GCV1 parse + segment→2-vertex expansion + axis swap (the reference logic to port)
- `src/qml_gui/Renderer/GCodeRenderer.cpp:290-309` — per-frame CPU filter (what D-26-02 eliminates)
- `src/qml_gui/Renderer/GCodeRenderer.cpp:564` — color mode application (CPU-side, already done by PreviewViewModel)
- `src/qml_gui/Renderer/GCodeRenderer.cpp:353-431` — marker geometry (D-26-04 reference)
- `src/qml_gui/Renderer/GCodeRenderer.h:36` — SegmentVertex (48-byte, reference for what fields exist)

### Data source / wire format
- `src/core/viewmodels/PreviewViewModel.cpp:45-65` — PackedSegment (80-byte GCV1 record: x1y1z1x2y2z2 rgb feedrate fan temp width layer_time accel extruder_id layer move)
- `src/core/viewmodels/PreviewViewModel.cpp:929-969` — GCV1 blob format (magic + count + PackedSegment[]) + recolorAndPackSegments (CPU color baking)
- `src/core/viewmodels/PreviewViewModel.h:17-69` — PreviewViewModel property surface (layer/move/color/legend/stats/marker)

### Integration points
- `src/qml_gui/pages/PreviewPage.qml:207-223` — GLViewport host with canvasType:CanvasPreview + all Preview bindings (ZERO change needed)
- `src/qml_gui/main_qml.cpp:82-95,124-129` — gate: GLViewport name maps to RhiViewport under OWZX_RHI_RENDERER

### Benchmark validation
- `tools/render_bench/` — validated QRhi segment pipeline at 1M segments; default --segments 1000000

### Rules
- `.codex/rules/source-truth-migration.md` — QML is binding-only; status terms; verification.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets (no need to rebuild)
- `RhiViewport` host — already has ALL Preview Q_PROPERTYs (previewData, layerMin/Max, moveEnd, showTravelMoves, showBed, gcodeViewMode, markerXYZ, showMarker). Phase 26 just consumes them.
- `RhiViewportRenderer` — camera uniform pipeline, ensureBuffer(), m_linePipeline (Lines topology), m_fillPipeline. Phase 26 adds a CanvasPreview branch.
- `rhi_viewport.vert/.frag` shaders — CameraBlock + vec3 position + vec4 color passthrough. Usable for segments as-is.
- `render_bench` — proves the QRhi Lines pipeline handles 1M segments; the segment shader exists.
- `CameraController` — orbit/pan/zoom/fit; produces viewMatrix/projMatrix. Reusable for Preview camera.

### Established Patterns (from Phase 23-25 SUMMARYs)
- Scene-data object (PrepareSceneData) holds CPU truth + dirty flags; renderer owns GPU resources; synchronize() mirrors CPU→GPU; render() uploads+draws.
- peek/take dirty flags; Static buffers via uploadStaticBuffer; Dynamic uniforms via updateDynamicBuffer; clip-space correction.
- Same QML type name across all 3 viewport impls (GLViewport/RhiViewport/SoftwareViewport) — property parity required.
- QML is binding-only; ViewModels own truth.

### Integration Points
- RhiViewportRenderer::synchronize() — currently reads m_previewBytes but discards. Phase 26 stores preview data + control props.
- RhiViewportRenderer::render() — currently bails on CanvasPreview (line 84). Phase 26 adds the branch.
- PreviewViewModel GCV1 blob → RhiViewport.previewData Q_PROPERTY (already bound in QML, no change).

</code_context>

<deferred>
## Deferred Ideas

- **GPU-side color mode computation** (shader uniform + palette texture) — deferred; CPU pre-baking is the established pattern and mode-switch latency is acceptable.
- **GCodeProcessorResult.moves direct consumption** — PreviewViewModel currently re-parses .gcode text (lossy). Switching to the structured moves vector is a separate refactor (Preview data-quality improvement, not rendering).
- **1M/5M performance benchmarking + instrumentation** — Phase 27.
- **Fallback hardening + canonical verification with QRhi present** — Phase 28.
- **Layer-height/line-width/flow/acceleration color modes requiring per-vertex scalar encoding** — if a mode can't be CPU-pre-baked efficiently (needs the raw scalar in-shader), defer to a follow-up; for Phase 26, rely on PreviewViewModel's existing recolorAndPackSegments which already handles all 13 modes.

</deferred>

---

*Phase: 26-preview-g-code-gpu-pipeline*
*Context gathered: 2026-06-27*
