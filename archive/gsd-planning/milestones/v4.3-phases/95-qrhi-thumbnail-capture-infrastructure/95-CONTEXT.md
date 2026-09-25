# Phase 95: QRhi Thumbnail Capture Infrastructure - Context

**Gathered:** 2026-07-10
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 95 implements the real QRhi thumbnail capture infrastructure, replacing
the `RhiViewport::requestThumbnailCapture` solid-color stub with real framebuffer
readback. This is the core technical phase of v4.3 — it builds the offscreen
render-target, MSAA-safe readback, and render-thread capture queue that Phase 96
(writer) and Phase 97 (round-trip) consume.

Phase 95 delivers (per the Phase 94 frozen decisions):
- Real QRhi texture readback capture reflecting the rendered scene, replacing the
  `#18222c` solid-color PNG stub (`RhiViewport.cpp:476-488`).
- An offscreen `QRhiTexture` render-target at thumbnail size (Option A frozen
  decision) + `QRhiResourceUpdateBatch::readBackTexture()`.
- Single-sample offscreen RT (MSAA resolve = no resolve needed, since the
  on-screen RT is multisampled at sample count 4 but the thumbnail RT is
  sample-count-1).
- A render-thread capture request queue mirroring the existing
  `m_fitRequestCount`/`m_viewPreset` synchronize() pattern: item sets a capture
  request, `synchronize()` copies it to the renderer, `render()` performs the
  readback, and a queued signal delivers the QImage back to the item thread.

Out of scope for Phase 95:
- 3MF write-side population (`PlateData::plate_thumbnail` / `StoreParams::thumbnail_data`) → Phase 96.
- Save-reload round-trip test → Phase 97.
- Mock path removal + final verification → Phase 98.
- Auto filament-map, CLI fixtures, D3D12, GLGizmoMeasure — future.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

</domain>

<decisions>
## Implementation Decisions

### Phase 94 Frozen Decisions (MUST honor — these are locked)

1. **QRhi readback = Option A (offscreen QRhiTexture RT + readBackTexture):**
   Render the scene into a separate offscreen `QRhiTexture` render-target at
   thumbnail size, then `QRhiResourceUpdateBatch::readBackTexture()` to get a
   `QRhiReadbackResult`. Do NOT use Option B (live RT color attachment readback).
   Rationale: mirrors upstream's dedicated thumbnail framebuffer; yields a
   single-sample thumbnail-sized texture; duplicated render pass only on rare
   capture frames.

2. **MSAA resolve = single-sample offscreen RT:**
   The thumbnail offscreen RT is sample-count-1, so NO resolve step is needed.
   (The on-screen RT is multisampled at `setSampleCount(4)` per
   `RhiViewport.cpp:47`, but that does not affect the separate thumbnail RT.)

3. **Render-thread queue = mirror m_fitRequestCount/m_viewPreset pattern:**
   - Item-side (RhiViewport, GUI thread): add capture-request fields (target
     plateIndex, size, pending flag), set by `requestThumbnailCapture()`.
   - `synchronize()` (`RhiViewportRenderer.cpp:35`) copies the pending request to
     the renderer.
   - `render()` (`RhiViewportRenderer::render`) performs the offscreen RT render
     + readback when a request is pending.
   - QImage delivered back to the item thread via a queued signal/connection
     (`thumbnailCaptured` signal at `RhiViewport.h:239`, `lastThumbnailData`
     Q_PROPERTY at `:69,313`).

### Claude's Discretion
- Exact thumbnail size (128 default, per existing `requestThumbnailCapture(int plateIndex, int size = 128)`).
- Whether to keep the existing `requestThumbnailCapture` Q_INVOKABLE signature or extend it.
- Internal structure of the offscreen RT (texture format, render-pass descriptor reuse vs new).
- How the scene is rendered into the offscreen RT (reuse the existing render code path with a different RT, or a dedicated compact render).

### Recommended approach (Claude's Discretion confirmed, noted for planning)
- Reuse the existing scene rendering (bed/plate/mesh) but target the offscreen
  QRhiTexture RT instead of the on-screen renderTarget(). This keeps a single
  scene-render code path parameterized by RT.
- The offscreen RT needs its own `QRhiRenderPassDescriptor` (compatible with the
  thumbnail texture format).
- readBackTexture delivers a `QRhiReadbackResult` with a completed QImage (Qt
  handles the format conversion); wrap it into the base64 PNG string the
  existing `lastThumbnailData` Q_PROPERTY carries, OR change to carry the QImage
  directly if the downstream (Phase 96) prefers QImage.
- Readback is asynchronous (completes on a later frame); the renderer must check
  the readback result on the NEXT render() call after issuing it, then emit
  thumbnailCaptured.

</decisions>

<code_context>
## Existing Code Insights

### Integration points (the stub to replace + its surroundings)
- `src/qml_gui/Renderer/RhiViewport.h:6,18` — `class RhiViewport : public QQuickRhiItem`.
- `src/qml_gui/Renderer/RhiViewport.h:229` — `Q_INVOKABLE void requestThumbnailCapture(int plateIndex, int size = 128)` (the stub entry point).
- `src/qml_gui/Renderer/RhiViewport.h:69,239,313` — `lastThumbnailData` Q_PROPERTY, `thumbnailCaptured()` signal, `m_lastThumbnailData`.
- `src/qml_gui/Renderer/RhiViewport.h:314-315` — `m_fitRequestCount`/`m_viewPreset` (the queue pattern template).
- `src/qml_gui/Renderer/RhiViewport.cpp:47` — `setSampleCount(4)` (on-screen MSAA; does NOT affect offscreen thumbnail RT).
- `src/qml_gui/Renderer/RhiViewport.cpp:415,435,442` — the synchronize() field-copy pattern.
- `src/qml_gui/Renderer/RhiViewport.cpp:476-488` — the solid-color stub to replace.
- `src/qml_gui/Renderer/RhiViewportRenderer.h:17,20,144` — `QQuickRhiItemRenderer` subclass, `m_renderPassDescriptor`.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:35` — `synchronize()`.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:88,210,270,276,506` — `renderTarget()`, `renderPassDescriptor()`, render pass via `cb->endPass()`.

### Qt QRhi readback API (the new capability)
- `QRhi::newTexture(QRhiTexture::RGBA8, QSize, 1, QRhiTexture::RenderTarget)` — create the offscreen texture.
- `QRhi::newTextureRenderTarget({ QPair{texture, QRhiTextureRenderTargetDescription::ColorLevel() } })` — offscreen RT.
- `QRhiResourceUpdateBatch::readBackTexture(QRhiReadbackDescription{texture}, QRhiReadbackResult*)` — the readback.
- `QRhiReadbackResult::format` + `data` — Qt converts to QImage when format is set.

### Downstream consumers (Phase 96 will use this)
- `src/core/services/ProjectServiceMock.cpp:4242` — current mock generator (the QPainter placeholder Phase 98 removes).
- `src/core/model/PartPlate.h:122-124,255` — `setThumbnail(QImage)` destination.

### Test surface
- `tests/QmlUiAuditTests.cpp` — add a slot asserting the stub is gone / real readback wired.
- `tests/ViewModelSmokeTests.cpp` — possibly add a capture-request smoke test.

</code_context>

<specifics>
## Specific Ideas

- The capture must prove end-to-end at runtime: load a model, call
  requestThumbnailCapture, get a non-solid-color QImage back. This is the smoke
  test for THUMBCAP-01.
- The offscreen RT render must reuse the existing scene rendering so the
  thumbnail reflects the actual model (not just bed grid).
- Readback asynchrony (completes next frame) must be handled — the renderer checks
  the pending readback result at the start of the next render() and emits
  thumbnailCaptured only when the QImage is ready.

</specifics>

<deferred>
## Deferred Ideas

- 3MF write-side population → Phase 96.
- Save-reload round-trip test → Phase 97.
- Mock generator removal + final verification → Phase 98.

</deferred>

---

*Phase: 95-qrhi-thumbnail-capture-infrastructure*
*Context gathered: 2026-07-10 (discuss skipped via workflow.skip_discuss; Phase 94 frozen decisions embedded)*
