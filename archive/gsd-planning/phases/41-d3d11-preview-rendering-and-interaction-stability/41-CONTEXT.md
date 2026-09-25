# Phase 41: D3D11 Preview Rendering and Interaction Stability - Context

**Gathered:** 2026-06-29
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 41 makes the Preview page's high-performance Qt renderer stable under the real interactions in the local import-to-G-code workflow. It consumes Phase 40's coherent `GCV1` Preview payload and focuses on D3D11 QRhi resource lifecycle, layer/move range updates, camera fit/orbit/pan/zoom, resize, toggles, plate/result switching, diagnostics, and performance guardrails.

This phase does not change slicing algorithms, parser semantics, export finalization, or promote Vulkan/D3D12 as the default. Upstream OrcaSlicer remains the source truth for Preview behavior, but the rendering implementation intentionally uses Qt's own QRhi/D3D11 path rather than upstream OpenGL/libvgcode internals.

</domain>

<decisions>

## Implementation Decisions

### Rendering Path

- D-41-01: Keep Windows default Preview rendering on `RhiViewport` through Qt QRhi with D3D11-first backend selection. `SoftwareViewport` remains only an initialization fallback, not the normal capable-system path.
- D-41-02: Do not reintroduce `QQuickFramebufferObject`/OpenGL as the default app path. `OWZX_OPENGL=1` remains an explicit legacy override.
- D-41-03: Vulkan and D3D12 remain explicit benchmark/experiment candidates. They are not promoted in Phase 41 because the user-facing defect is on the current main workflow and D3D11 is the known stable Qt-native path.

### Preview Payload and Draw Semantics

- D-41-04: `PreviewViewModel::gcodePreviewData` and the `GCV1` packed payload remain the data contract for renderer segments.
- D-41-05: Layer/move slider interaction must update draw range over the existing uploaded segment buffer. It must not trigger unnecessary payload rebuilds or fallback viewport swaps.
- D-41-06: Draw-range filtering may continue to use a contiguous vertex span if visible layers/moves are contiguous after Phase 40 packing, but it must clamp invalid ranges and never draw from stale or released GPU buffers.

### Resource Lifecycle

- D-41-07: QRhi resource release, renderer initialization, render-target rebuild, page switching, and resize must reset Preview GPU upload state consistently. CPU staging data may stay cached; GPU upload flags must not claim a released buffer is live.
- D-41-08: Camera-only, marker-only, bed-toggle, layer-range, move-range, and viewport resize updates must preserve visible valid toolpath data and should upload only the resources they actually invalidate.
- D-41-09: Diagnostics should be specific enough to identify selected backend, payload size, segment count, draw range, upload state, and first-frame/per-frame timing without flooding normal logs.

### Verification

- D-41-10: Add RED tests before production renderer changes. Prefer deterministic source/audit tests for QRhi lifecycle invariants and focused runtime smoke where headless GPU capture is unreliable.
- D-41-11: Canonical verification remains the only accepted build/test command: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

</decisions>

<code_context>

## Existing Code Insights

### Reusable Assets

- `src/qml_gui/Renderer/RhiViewport.{h,cpp}` hosts `QQuickRhiItem`, QML properties, Preview camera fitting from `GCV1`, mouse orbit/pan/zoom, and `update()` calls for slider/toggle/camera changes.
- `src/qml_gui/Renderer/RhiViewportRenderer.{h,cpp}` owns QRhi pipelines, buffers, camera uniform upload, `GCV1` parsing, Preview segment upload, and draw-range filtering.
- `src/qml_gui/main_qml.cpp` selects QRhi by default, sets D3D11-first backend policy through `RhiBackendSelector`, and registers `RhiViewport` as `OWzxGL.GLViewport` when QRhi initializes.
- `src/qml_gui/pages/PreviewPage.qml` binds Preview data and interaction state into `GLViewport`.
- `tests/QmlUiAuditTests.cpp` already guards the RHI normal path, exact draw spans, camera fit hook, and no direct `SoftwareViewport` in `PreviewPage.qml`.

### Current Gaps

- `RhiViewportRenderer::releaseResources()` releases `m_previewSegmentBuffer` but does not reset `m_previewSegmentBufferUploaded`, `m_previewSegmentBufferBytes`, `m_previewSegmentVertexCount`, or Preview first-frame timing state. After QRhi resource rebuild, the renderer can think the released Preview buffer is already uploaded and draw nothing.
- `initialize()` calls `releaseResources()` and then only resets general scene flags. Preview-specific upload state is not explicitly rearmed.
- `computePreviewDrawRange()` assumes `m_layerMin <= m_layerMax` and can return no draw if interaction briefly produces an inverted or out-of-range selection. Slider code normally clamps, but renderer should still be defensive.
- Preview-specific show states are partially consumed: `showTravelMoves` is payload-driven by `PreviewViewModel`, while `showBed` and `showMarker` are QML-bound but renderer-side Preview drawing currently only draws segments. Phase 41 should at least ensure these toggles do not disturb toolpath buffers; full visual bed/marker rendering can be bounded if not required for blanking stability.
- Existing tests are source-audit heavy. They do not currently guard the resource-release bug that matches the user's observed blanking after slider/camera interaction.

### Upstream Behavior Reference

- `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.cpp` keeps Preview sliders as state over the current `GCodeProcessorResult` and reloads G-code preview only when the result changes.
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp` updates G-code viewer layer ranges and sequential move ranges from dirty slider state, then renders the same loaded Preview data.
- Upstream camera interaction updates the canvas camera and redraws; it does not invalidate the G-code result or switch renderer classes.

</code_context>

<specifics>

## Specific Ideas

- Add source-audit RED coverage that fails until `releaseResources()` fully resets Preview GPU upload state and `initialize()` leaves Preview ready to reupload existing CPU segment data.
- Add source/audit or focused unit coverage that range/camera/toggle setters call `update()` but do not mutate `m_previewData`.
- Implement a small private helper such as `resetPreviewGpuState(bool keepCpuStaging)` to centralize buffer/upload/timing reset.
- Mark preview buffer upload dirty whenever the payload changes, resource state is released, or the byte size changes; never set upload true unless `uploadStaticBuffer()` was queued for a live QRhi buffer.
- Clamp draw range defensively: empty payload or `moveEnd <= 0` draws zero; inverted layer bounds normalize or safely return based on source-truth slider semantics; out-of-range bounds cannot use stale offsets.
- Add concise `[RHI-PREVIEW]` diagnostics around backend selection and preview upload/draw range.

</specifics>

<deferred>

## Deferred Ideas

- D3D12 crash root-cause and Vulkan default promotion remain separate backend work.
- Pixel-perfect visual regression for G-code preview remains Phase 43 unless Phase 41 needs an extra manual UAT artifact.
- Full upstream Preview rendering parity beyond the v3.4 local workflow, including shell overlay sophistication and all marker visuals, can continue after the main workflow is stable.

</deferred>
