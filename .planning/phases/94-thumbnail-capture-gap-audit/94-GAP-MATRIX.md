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
| THUMB-MOCK-GENERATOR | `ProjectServiceMock.cpp:4242 generatePlateThumbnail()` (QPainter flat-color placeholder); `:4362 generatePlateThumbnailVariant()`; `:4372 generateTopDownThumbnail()`. | POPULATED IN TASK 02. | `src/core/services/ProjectServiceMock.cpp:4242,4362,4372`. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. |
| THUMB-REQUEST-STUB | `RhiViewport.cpp:476-488 requestThumbnailCapture()` — solid-color `#18222c` PNG stub, no GL/RHI involvement. Entry `Q_INVOKABLE requestThumbnailCapture(int plateIndex, int size = 128)` at `RhiViewport.h:229`. | POPULATED IN TASK 02. | `src/qml_gui/Renderer/RhiViewport.cpp:476-488`; `src/qml_gui/Renderer/RhiViewport.h:229`. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. |
| THUMB-WRITE-PLATE | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. |
| THUMB-WRITE-STOREPARAMS | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. |
| THUMB-READ-SIDE | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. |
| THUMB-RHI-READBACK | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. |
| THUMB-MSAA-RESOLVE | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. |
| THUMB-RT-QUEUE | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. | POPULATED IN TASK 03. |
| THUMB-PLATE-CACHE | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. |
| THUMB-CLEANUP | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. | POPULATED IN TASK 02. |
