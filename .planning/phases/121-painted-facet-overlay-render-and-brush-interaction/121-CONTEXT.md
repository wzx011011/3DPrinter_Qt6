# Phase 121: Painted-Facet Overlay Render And Brush Interaction - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close PAINT-02 (colored-facet overlay on QRhi/D3D11 + Software QPainter mirror) + PAINT-03 (brush interaction: radius sphere/circle + size + paint/erase/smart-fill). Phase 120 built PaintEngine (getFacets API); Phase 121 renders those facets + completes the brush UX.

</domain>

<decisions>
## Implementation Decisions (from rendering research)

### Reuse mesh pipeline — no new shader/pipeline
- Overlay uses `GizmoVertex` format (7 floats: xyz + rgba), the existing `m_fillPipeline` (or `m_translucentFillPipeline` for brush cursor), and `rhi_viewport.vert/frag.qsb` (pure vertex-color passthrough). Zero new shaders.
- New: `m_paintOverlayBuffer` + `uploadPaintOverlayBuffer` + `renderPaintOverlay`. Render call point: after model mesh draw, before highlight (RhiViewportRenderer.cpp:335).

### Reverse data channel (the core gap)
EditorViewModel → RhiViewport has NO paint-facet channel today. Build it via a QByteArray Q_PROPERTY (mirrors the existing `meshData` pattern):
- EditorViewModel: `Q_PROPERTY(QByteArray paintOverlayData READ paintOverlayData NOTIFY paintDataChanged)` — getter flattens painted facets (Enforcer + Blocker states; MMU: Extruder1..N) into `[state, vx,vy,vz ×3]` bytes, world-transformed.
- RhiViewport: `Q_PROPERTY(QByteArray paintOverlayData ...)` setter → `m_paintOverlayData` + update().
- PreparePage.qml: bind `paintOverlayData: editorVm.paintOverlayData`.
- synchronize pulls + parses → uploadPaintOverlayBuffer (dirty-gated).

### Brush parameter wiring
`emitPaintPickIfActive` (RhiViewport.cpp:1034-1038) hardcodes brushRadius=2.0/cursorType=Sphere/paintState. Replace with RhiViewport Q_PROPERTYs (brushRadius/cursorType/paintState) bound from editorVm.supportPaint*/seamPaint*/mmu* in QML (by gizmoMode).

### Color mapping (EnforcerBlockerType → color)
- Support/Seam: Enforcer=light green (0.5,1,0.5,1), Blocker=light red (1,0.5,0.5,1) — upstream GLGizmoPainterBase.
- MMU: per-extruder filament colors (needs editorVm to expose extruders_colors — add Q_PROPERTY).

### Brush cursor (sphere first)
Phase 121 does the sphere cursor (translucent, follows mouse, radius=brushRadius). Screen-space circle cursor (stipple) is complex in QRhi — defer or approximate with line segments.

### SoftwareViewport mirror
Add painted faces to the `faces` vector in paintScene (SoftwareViewport.cpp:460-483) for depth-sorted QPainter rendering.

</decisions>

<specifics>
## Code Access Points
- RhiViewportRenderer.cpp:236 (render), :314-367 (draw block — insert renderPaintOverlay at :335), :938-965 (uploadModelBuffer template), .h:108-160 (buffer member pattern).
- GizmoVertex.h:12 (vertex format).
- shaders/rhi_viewport.{vert,frag} (reuse).
- PaintEngine.cpp:89-104 (getFacets).
- EditorViewModel.h:530-547 (brush Q_PROPERTYs exist), .h:1028 (paintDataChanged), .cpp:2519-2631 (paintAtFacet).
- RhiViewport.h:310-315 (paintPickRequested), .cpp:996-1044 (emitPaintPickIfActive hardcoded brush).
- SoftwareViewport.cpp:428-598 (paintScene).
- PreparePage.qml seam panel (:2514-2606) as Support-panel template (has radius CxSlider).
- TriangleSelector.hpp:13-38 (EnforcerBlockerType).

## Source-Truth Anchors
- Upstream render_triangles: GLGizmoPainterBase.cpp:76-132.
- Upstream enforcers/blockers color: GLGizmoPainterBase.cpp:1199-1200.
- Upstream cursor sphere: GLGizmoPainterBase.hpp:231-234.

</specifics>

<deferred>
## Deferred Ideas
- Screen-space circle cursor (stipple) — complex in QRhi; sphere cursor covers Phase 121.
- Smart-fill (bucket) full algorithm — Phase 120 PaintEngine has the upstream seed/bucket API; UI toggle is Phase 121, full flood-fill may need tuning.
- Support/Seam/MMU slice integration → Phase 122/123.

</deferred>
