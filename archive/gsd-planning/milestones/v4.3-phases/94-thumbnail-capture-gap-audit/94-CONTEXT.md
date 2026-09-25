# Phase 94: Thumbnail Capture Gap Audit - Context

**Gathered:** 2026-07-10
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 94 is a read-only audit phase for the v4.3 Real Thumbnail Capture And 3MF
Round-Trip milestone. It freezes the capture + writer region map before
implementation: mapping the current mock paths, the deferred write-side gaps,
upstream source anchors, the QRhi readback approach, MSAA resolve strategy, and
verification expectations.

This phase does not modify production source code. It produces the canonical
v4.3 thumbnail capture gap matrix (`94-GAP-MATRIX.md`) that Phase 95-98 execute
against, modeled after the Phase 84/89 gap matrix structure.

In scope:
- Mock thumbnail paths: `generatePlateThumbnail` (QPainter placeholder), `requestThumbnailCapture` (solid-color PNG stub).
- 3MF write-side gaps: `saveProject` omitting `StoreParams::thumbnail_data` + `PlateData::plate_thumbnail`.
- 3MF read side (already complete): `plate->plate_thumbnail` → QImage extraction.
- Upstream writer anchors: `store_bbs_3mf` thumbnail path, `_add_thumbnail_file_to_archive`, `PartPlate::store_to_3mf_structure`.
- QRhi readback design options: offscreen `QRhiTexture` RT render + `readBackTexture` vs live RT color attachment readback.
- MSAA resolve strategy (sample count > 1).
- Render-thread capture request queue + QImage callback design (item → renderer → item).

Out of scope:
- Production code changes.
- Auto filament-map + wipe-tower, CLI fixtures, D3D12, full GLGizmoMeasure engine.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion (discuss skipped — use ROADMAP goal + exploration findings + codebase conventions)

The gap matrix must capture the following (from pre-planning exploration):

1. **Mock paths to replace:**
   - `src/core/services/ProjectServiceMock.cpp:4242 generatePlateThumbnail()` — QPainter flat-color placeholder (dark bed grid + rounded-rectangle object blocks + green "sliced" checkmark). Returns base64 PNG.
   - `src/qml_gui/Renderer/RhiViewport.cpp:476-488 requestThumbnailCapture()` — solid-color `#18222c` PNG stub. No GL/RHI involvement.
   - `ProjectServiceMock.cpp:4362 generatePlateThumbnailVariant()` + `:4372 generateTopDownThumbnail()` (THUMB-01 variants).

2. **3MF write-side gaps (the v3.2 THUMB-02 deferred write):**
   - `ProjectServiceMock.cpp:5089-5093` — `buildPlateDataList` comment: `plate_thumbnail` pixel population deferred to THUMB-03 (writer's PNG encoding path coupled to real GL capture).
   - `ProjectServiceMock.cpp:5127-5135` — `saveProject` comment: `StoreParams::thumbnail_data` (read by writer at `bbs_3mf.cpp:6133`) intentionally NOT populated because `_add_thumbnail_file_to_archive` throws a non-std exception on the Qt6 mock pipeline.

3. **3MF read side (already complete — Phase 97 verifies round-trip):**
   - `ProjectServiceMock.cpp:5455-5466` — extracts `plate->plate_thumbnail` (populated by `bbs_3mf.cpp:1640 _extract_from_archive`) → QImage via `Format_RGBA8888`.
   - `ProjectServiceMock.cpp:5654-5660` — applies `pendingPlateThumbnails_[pi]` to `PartPlate::setThumbnail()` on rebuild.

4. **QRhi readback design options (freeze the chosen approach in THUMBAUDIT-02):**
   - **Option A:** Render scene into a separate offscreen `QRhiTexture` render-target at thumbnail size, then `QRhiResourceUpdateBatch::readBackTexture()`. Clean separation, controllable size, but duplicates the render pass.
   - **Option B:** Read back the live `renderTarget()` color attachment after a render pass. Reuses existing render, but the RT may be multisampled (need resolve) and is sized to the viewport (not thumbnail size — needs downscale).
   - The MSAA resolve is needed either way (sample count > 1 per `RhiViewport.cpp:43`).

5. **Render-thread capture queue design (THUMBCAP-03):**
   - `requestThumbnailCapture` lives on `RhiViewport` (QML item, GUI thread) but readback MUST happen inside `RhiViewportRenderer::render()` (render thread).
   - Pattern to mirror: existing `m_fitRequestCount`/`m_viewPreset` request fields (item → renderer via `RhiViewportRenderer::synchronize()`).
   - Need: a capture request flag + target plateIndex/size set on item, synchronized to renderer, which performs the readback and delivers QImage back via a queued signal/connection to the item thread.

6. **PartPlate thumbnail cache (the destination):**
   - `src/core/model/PartPlate.h:122-124,255` — `thumbnail()`/`setThumbnail(QImage)`/`hasThumbnail()`, Qt-native `QImage m_thumbnail`, invalidated on content mutation.

</decisions>

<code_context>
## Existing Code Insights

### Upstream behavior truth anchors (port/integration targets)
- `third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.cpp:6133` — writer reads `StoreParams::thumbnail_data`.
- `third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.cpp:1640` — `_extract_from_archive` populates `plate->plate_thumbnail` on load.
- `_add_thumbnail_file_to_archive` — PNG encoding path that throws on Qt6 mock pipeline (the blocker).
- `PartPlate::store_to_3mf_structure` — the `buildPlateDataList` source-truth anchor.

### Current Qt integration points
- `src/qml_gui/Renderer/RhiViewport.h:6,18,229,239,313` — `QQuickRhiItem` subclass, `requestThumbnailCapture` Q_INVOKABLE, `lastThumbnailData` Q_PROPERTY, `thumbnailCaptured` signal, `m_lastThumbnailData` member.
- `src/qml_gui/Renderer/RhiViewport.cpp:476-488` — the stub to replace.
- `src/qml_gui/Renderer/RhiViewportRenderer.h:17,20,144` — `QQuickRhiItemRenderer` subclass, `m_renderPassDescriptor`.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:88,210,270,276,506` — uses `renderTarget()`, `renderPassDescriptor()`, render pass via `cb->endPass()`.
- `src/core/services/ProjectServiceMock.cpp:4242,4362,4372,5089,5100,5127,5455,5654` — mock generator + save/read paths.
- `src/core/model/PartPlate.h:122-124,255` — thumbnail cache.

### Prior gap-audit pattern (template)
- `.planning/phases/89-assembleview-source-truth-gap-audit/89-GAP-MATRIX.md` (now archived under milestones/v4.2-phases/) — the structure to replicate.
- `.planning/milestones/v3.2-REQUIREMENTS.md` — THUMB-01/02/03 exact definitions.

</code_context>

<specifics>
## Specific Ideas

- The matrix must explicitly classify which mock paths are "remove" vs "keep-as-fallback" (e.g., the top-down 2D footprint variant may stay as a non-GL option).
- The QRhi readback design decision (Option A offscreen vs Option B live RT) must be frozen here because it drives Phase 95's implementation structure.
- The MSAA resolve approach must be documented (resolve-before-readback, or render-to-single-sample offscreen RT).
- The render-thread queue design must mirror the existing `m_fitRequestCount`/`m_viewPreset` synchronize() pattern so the executor has a concrete template.

</specifics>

<deferred>
## Deferred Ideas

- Implementation of any capture/writer/round-trip code is deferred to Phase 95-98.
- Runtime pixel-parity proof is deferred to Phase 98.
- Auto filament-map, CLI fixtures, D3D12, GLGizmoMeasure — future milestones.

</deferred>

---

*Phase: 94-thumbnail-capture-gap-audit*
*Context gathered: 2026-07-10 (discuss skipped via workflow.skip_discuss; exploration findings embedded)*
