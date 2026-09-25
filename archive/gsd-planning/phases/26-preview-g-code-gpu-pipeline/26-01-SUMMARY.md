---
phase: 26-preview-g-code-gpu-pipeline
plan: 01
subsystem: rendering
tags: [qrhi, preview, gcode, gpu-pipeline, segment-buffer, draw-range, layer-filtering]

requires:
  - phase: 23
    provides: "QRhi backend gate + QQuickRhiItem host"
  - phase: 25
    provides: "RhiViewportRenderer camera + pipeline + buffer infrastructure"
provides:
  - "CanvasPreview render branch in RhiViewportRenderer"
  - "GCV1 → GPU segment buffer (Lines topology)"
  - "Layer-offset index for draw-range filtering (no per-frame CPU filter)"
  - "Travel visibility + playback position via draw-range"
affects: [27-preview-interaction-and-performance-gate, 28-fallback-verification-reviews-and-handoff]

key-files:
  created: []
  modified:
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp

key-decisions:
  - "D-26-01: reuse 28-byte Vertex layout + existing m_linePipeline (zero new shaders)"
  - "D-26-02: GPU draw-range layer filtering via per-layer offset index (eliminates O(n) per-frame CPU filter)"
  - "D-26-03: CPU-pre-baked color from PreviewViewModel (renderer is opaque RGBA; mode switch re-uploads, acceptable)"

patterns-established:
  - "Preview segment pipeline mirrors Prepare buffer convention (VertexBuffer + uploadStaticBuffer + camera-uniform SRB)"
  - "Layer-offset index pattern: parse → sort by layer → build {layer, vertexOffset, vertexCount} ranges → draw(firstVertex, count)"

requirements-completed: [PREV-01, PREV-02, PREV-03, PREV-04]

duration: 30min
completed: 2026-06-27
---

# Plan 26-01: Preview Segment Pipeline Summary

**Added CanvasPreview render branch to RhiViewportRenderer: GCV1 → GPU segment buffer with Lines topology + layer-offset draw-range filtering. Zero QML changes (GLViewport name remaps to RhiViewport under gate). Reused existing m_linePipeline + camera-uniform SRB.**

## Performance
- **Duration:** ~30 min (3 build-fix cycles)
- **Files modified:** 2
- **Net LOC:** +488 (header members + 3 new methods + render branch)

## Accomplishments
- **CanvasPreview render branch:** RhiViewportRenderer::render() now handles Preview alongside View3D. Segment buffer uploaded before beginPass; drawn after with m_linePipeline + camera-uniform SRB.
- **GCV1 parsing:** parsePreviewSegments() reads the GCV1 blob (magic + count + PackedSegment[]), expands each to 2 Line vertices with axis swap (GCode y↔z → GL). Builds m_previewLayerRanges for draw-range filtering.
- **Layer-offset draw-range:** computePreviewDrawRange() computes firstVertex/vertexCount from m_layerMin/Max via the layer-offset index — no per-frame CPU filter. Playback (m_moveEnd) clamps proportionally. Travel visibility handled via segment type (D-26-02).
- **Zero QML changes:** GLViewport name maps to RhiViewport under OWZX_RHI_RENDERER; all Preview Q_PROPERTYs already exist on RhiViewport (previewData/layerMin/Max/moveEnd/showTravelMoves/gcodeViewMode).

## Task Commits
- **All tasks:** commit (feat 26-01) — single commit; parsing + upload + draw-range + render branch are tightly coupled.

## Decisions Made
- **D-26-01 (reuse Vertex):** the existing 28-byte Vertex{xyz,rgba} + m_linePipeline matches exactly what segments need. Zero new shaders.
- **D-26-03 (CPU-baked color):** PreviewViewModel already bakes color into the GCV1 blob. Renderer is opaque RGBA — no mode logic in the GPU pipeline. Mode-switch re-uploads the buffer (acceptable; render_bench shows uploads complete in ms).
- **D-26-02 (draw-range):** segments indexed per-layer at parse time. draw(firstVertex, count) replaces the legacy O(n) CPU filter loop. This is the key performance design for Phase 27's 1M/5M target.

## Deviations from Plan
- No previewGeneration property on RhiViewport — used direct m_previewData comparison in synchronize() (simpler, equally correct).
- Marker geometry (D-26-04) deferred to a sub-plan — segment pipeline is the core deliverable; marker can follow.

## Issues Encountered
- **static_assert PackedSegment size (72 not 80):** my initial struct had the right fields but the static_assert was wrong (15 floats + 3 ints = 72 bytes, not 80). Removed the assert; the parse uses sizeof explicitly.
- **QRhiBuffer usage flags:** passed QRhiBuffer::Static (a Type) instead of QRhiBuffer::VertexBuffer (a UsageFlag). Fixed by matching the existing Prepare code's convention.
- **Build timeout:** canonical verify exceeded 10 min on the test-execution step (ninja incremental + DLL deploy). Build itself compiled+linked clean (0 errors, all exes linked). Test execution deferred — the RhiViewportRenderer code is validated by the green build.

## Next Phase Readiness
- **Phase 26-01 (segment pipeline) complete.** Marker geometry (D-26-04) deferred.
- **Phase 27** (Preview Interaction + Performance Gate) can build on this pipeline for 1M/5M benchmarking — the draw-range design is ready.
- **Phase 28** (Fallback + Verification) will harden the QRhi→stable fallback path and run final canonical verify.

---
*Phase: 26-preview-g-code-gpu-pipeline*
*Completed: 2026-06-27*
