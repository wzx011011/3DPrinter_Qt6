# Phase 94 Thumbnail Capture Gap Matrix

**Scope:** v4.3 Real Thumbnail Capture And 3MF Round-Trip. The thumbnail capture
surface (mock generator + solid-color stub), the deferred 3MF write-side gaps,
the upstream writer anchors, the QRhi readback approach, the MSAA resolve
strategy, the render-thread capture queue design, the destination PartPlate
cache, and the cleanup candidates only. No LAN/device/cloud/network/Monitor/
ModelMall/camera/printer-hardware, D3D12/Vulkan, libslic3r slicing algorithm,
auto filament-map, CLI fixture, or GLGizmoMeasure-engine scope is in Phase 94.

## Summary

Phase 94 is the v4.3 source-truth audit. Its job is to freeze the thumbnail
capture + 3MF writer region map before implementation. The current Qt surface
ships only mock placeholders: a QPainter flat-color `generatePlateThumbnail`
generator (`ProjectServiceMock.cpp:4242`) and a solid-color
`requestThumbnailCapture` stub (`RhiViewport.cpp:476-488`) that produce no
real GPU pixels. The 3MF write side explicitly omits both
`PlateData::plate_thumbnail` and `StoreParams::thumbnail_data` because the
upstream writer's PNG encoding path throws on the Qt6 mock pipeline. This
matrix is the canonical routing artifact for Phase 95-98.

This phase is read-only with respect to production source: it modifies
documentation only and produces no QML/C++ changes. Runtime pixel parity and
visual proof are owned by Phase 98; Phase 94 does not claim them.

## Current State (from pre-planning exploration)

The exact mock paths and write-side gaps, with line citations read from the
current Qt6 source:

- `src/core/services/ProjectServiceMock.cpp:4242 generatePlateThumbnail()` —
  QPainter flat-color placeholder (dark bed grid + rounded-rectangle object
  blocks + green "sliced" checkmark). Returns a base64-encoded PNG string. No
  GL/RHI involvement.
- `src/qml_gui/Renderer/RhiViewport.cpp:476-488 requestThumbnailCapture()` —
  solid-color `#18222c` PNG stub. Constructs a `QImage` filled with a flat
  color and base64-encodes it; `emit thumbnailCaptured()` fires but no rendered
  scene is captured.
- `src/core/services/ProjectServiceMock.cpp:4362 generatePlateThumbnailVariant()`
  + `:4372 generateTopDownThumbnail()` — THUMB-01 variant generators
  (`variant=0` delegates to the main generator; `variant=1` is the top-down 2D
  footprint via QPainter).
- `src/core/services/ProjectServiceMock.cpp:5089-5093` — `buildPlateDataList`
  comment: `PlateData::plate_thumbnail` pixel population is deferred to THUMB-03
  because the writer's PNG encoding path is coupled to real GL capture. (The
  per-plate QImage cache + variant generation are implemented; only write-side
  pixel persistence is missing.)
- `src/core/services/ProjectServiceMock.cpp:5127-5135` — `saveProject` comment:
  `StoreParams::thumbnail_data` (read by the writer at `bbs_3mf.cpp:6133`) is
  intentionally NOT populated because `_add_thumbnail_file_to_archive` throws a
  non-std exception on the Qt6 mock pipeline.
- `src/core/services/ProjectServiceMock.cpp:5455-5466` — READ side already
  complete: extracts `plate->plate_thumbnail` (populated upstream by
  `bbs_3mf.cpp:1640 _extract_from_archive`) into a `QImage` via
  `QImage::Format_RGBA8888`.
- `src/core/services/ProjectServiceMock.cpp:5654-5660` — READ side already
  complete: applies `pendingPlateThumbnails_[pi]` to
  `PartPlate::setThumbnail()` on rebuild.

The destination thumbnail cache is `src/core/model/PartPlate.h:122-124,255`
(`thumbnail()`/`setThumbnail(QImage)`/`hasThumbnail()` over a Qt-native
`QImage m_thumbnail`, invalidated on every content mutation at `:116,157,163`).

## Canonical Region Matrix

| Region | Current Mock Path | Upstream Anchor | Qt Integration Point | Decision | Gap | Severity | Owner Phase | Requirement | Verification |
|---|---|---|---|---|---|---|---|---|---|
| THUMB-MOCK-GENERATOR | `ProjectServiceMock.cpp:4242 generatePlateThumbnail()` (QPainter flat-color placeholder: dark bed grid + rounded-rectangle object blocks + green "sliced" checkmark, returns base64 PNG); `:4362 generatePlateThumbnailVariant()` (variant=0 delegates to main, variant=1 delegates to top-down); `:4372 generateTopDownThumbnail()` (top-down 2D footprint via QPainter). | `PartPlate::store_to_3mf_structure` is the `buildPlateDataList` source-truth anchor (the generator's output flows into `PlateData` upstream). | `src/core/services/ProjectServiceMock.cpp:4242,4362,4372`. | Remove (main flat-color generator replaced by real GL capture); `generateTopDownThumbnail` (`:4372`) noted as a possible keep-as-fallback (non-GL 2D footprint variant). | Main generator draws a flat-color stand-in instead of the rendered scene; variant generator routes 2D/3D to flat placeholders. | High | Phase 98 | THUMBVERIFY-01 | Source audit proving the main generator is removed (or top-down classified as fallback) in Phase 98; downstream runtime visual evidence owned by Phase 98. |
| THUMB-REQUEST-STUB | `RhiViewport.cpp:476-488 requestThumbnailCapture()` — solid-color `#18222c` PNG stub, no GL/RHI involvement. Entry `Q_INVOKABLE requestThumbnailCapture(int plateIndex, int size = 128)` at `RhiViewport.h:229`. | Upstream real GL capture (no single upstream line): the Qt entry point is `requestThumbnailCapture` at `RhiViewport.h:229`; upstream renders a dedicated thumbnail via a separate framebuffer (see Frozen Decisions THUMB-RHI-READBACK Option A). | `src/qml_gui/Renderer/RhiViewport.cpp:476-488`; `src/qml_gui/Renderer/RhiViewport.h:229` (Q_INVOKABLE entry + `thumbnailCaptured` signal at `:239`). | Replace (solid-color stub replaced by real QRhi texture readback). | Stub returns a flat-color PNG; no rendered scene (bed/plate/mesh/gizmos) is captured. | Critical | Phase 95 | THUMBCAP-01 | Source audit proving `requestThumbnailCapture` performs real readback; Phase 98 runtime screenshot reflecting the rendered scene. |
| THUMB-WRITE-PLATE | `ProjectServiceMock.cpp:5089-5093` `buildPlateDataList` comment: `PlateData::plate_thumbnail` pixel population deferred because the writer's PNG encoding path is coupled to real GL capture. | `bbs_3mf.cpp:6137` (`_add_thumbnail_file_to_archive(archive, *thumbnail_data[index], "Metadata/plate", index, true)` writes per-plate thumbnail into the 3MF archive); `bbs_3mf.cpp:5879` (`_add_thumbnail_file_to_archive` declaration). | `src/core/services/ProjectServiceMock.cpp:5089-5093` (buildPlateDataList omits `PlateData::plate_thumbnail`); `:5100` (saveProject entry). | Add (populate `PlateData::plate_thumbnail` with real captured pixels on save). | Per-plate thumbnail pixels are not written into the 3MF archive (v3.2 THUMB-02 deferred write). | Critical | Phase 96 | THUMBWRITE-01 | Source audit proving `buildPlateDataList` populates `PlateData::plate_thumbnail`; Phase 97 round-trip test asserting pixels survive save→reload. |
| THUMB-WRITE-STOREPARAMS | `ProjectServiceMock.cpp:5127-5135` `saveProject` comment: `StoreParams::thumbnail_data` intentionally NOT populated because `_add_thumbnail_file_to_archive` throws a non-std exception on the Qt6 mock pipeline. | `bbs_3mf.cpp:6133` (`for (index = 0; index < thumbnail_data.size(); index++)` reads `StoreParams::thumbnail_data`). | `src/core/services/ProjectServiceMock.cpp:5127-5135` (`StoreParams params;` omits `thumbnail_data`). | Add (populate `StoreParams::thumbnail_data` with a real captured project thumbnail). | Project-level thumbnail is not written into the 3MF archive (v3.2 THUMB-02 deferred write). | Critical | Phase 96 | THUMBWRITE-02 | Source audit proving `saveProject` populates `StoreParams::thumbnail_data`; Phase 97 round-trip test. |
| THUMB-READ-SIDE | READ side already complete — `ProjectServiceMock.cpp:5455-5466` extracts `plate->plate_thumbnail` (populated upstream by `bbs_3mf.cpp:1640`) to `QImage` via `Format_RGBA8888`; `:5654-5660` applies `pendingPlateThumbnails_[pi]` via `PartPlate::setThumbnail()`. | `bbs_3mf.cpp:1640` (`_extract_from_archive` populates `plate->plate_thumbnail.pixels` on load). | `src/core/services/ProjectServiceMock.cpp:5455-5466` (extract to QImage); `:5654-5660` (apply via `PartPlate::setThumbnail`). | Preserve (already complete; Phase 97 verifies the round-trip, not re-implemented here). | None for the read side — once the write side lands (Phase 96), saved thumbnails will round-trip through the existing read path. | Low | Phase 97 | THUMBRT-01 | Source audit confirming the read path is unchanged; Phase 97 round-trip test asserting reloaded `PartPlate::thumbnail()` matches saved pixels. |
| THUMB-RHI-READBACK | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. |
| THUMB-MSAA-RESOLVE | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. |
| THUMB-RT-QUEUE | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. |
| THUMB-PLATE-CACHE | The destination cache is already in place: `PartPlate.h:122-124` (`thumbnail()`/`setThumbnail(QImage)`/`hasThumbnail()`), `:255` (`QImage m_thumbnail`); invalidated on content mutation at `:116` (setLocked), `:157` (addInstance), `:163` (removeInstance). | `PartPlate::thumbnail_data` destination (upstream `PartPlate.hpp` cache that the writer reads and the loader populates). | `src/core/model/PartPlate.h:122-124,255` (`thumbnail()`/`setThumbnail(QImage)`/`hasThumbnail()`/`m_thumbnail`); invalidation at `:116,157,163`. | Preserve (the destination cache is already in place). | None for the cache itself — it is ready to receive real captured pixels from Phase 95. | Low | Phase 97 | THUMBRT-01 | Source audit confirming the cache API + invalidation hooks are unchanged; Phase 97 round-trip test asserting `PartPlate::thumbnail()` survives save→reload. |
| THUMB-CLEANUP | Removal candidates: the mock generator (`ProjectServiceMock.cpp:4242,4362,4372`) and the solid-color stub (`RhiViewport.cpp:476-488`) once the real capture path lands. | No-dead-paths rule (No Deprecated UI Rule from PROJECT.md): once the real capture path is the sole source, the mock paths must leave no dead/disconnected code. | `src/core/services/ProjectServiceMock.cpp:4242,4362,4372`; `src/qml_gui/Renderer/RhiViewport.cpp:476-488`. | Remove (mock paths) — owned by Phase 95/96 (replacement lands there); Phase 98 audits no dead/disconnected paths remain. | Mock generator + solid-color stub remain as the only thumbnail paths today; they become dead once the real path lands. | High | Phase 98 | THUMBVERIFY-01 | Source audit proving no dead/disconnected mock paths remain after Phase 95/96 replacements; Phase 98 canonical verifier + regression ctest. |
