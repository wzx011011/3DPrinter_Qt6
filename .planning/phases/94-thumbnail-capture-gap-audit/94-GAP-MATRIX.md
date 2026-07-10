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
| THUMB-RHI-READBACK | N/A (no current readback code in repo — the solid-color stub at `RhiViewport.cpp:476-488` does not read back GPU pixels). | Upstream OrcaSlicer renders a dedicated thumbnail via a separate framebuffer, which maps to Option A (offscreen RT) below. | Entry `RhiViewport.h:229` (`requestThumbnailCapture` Q_INVOKABLE); readback runs inside `RhiViewportRenderer::render()` (render thread). See Frozen Decisions for the locked Option A choice. | Freeze Option A (offscreen QRhiTexture RT at thumbnail size + `QRhiResourceUpdateBatch::readBackTexture()`). See Frozen Decisions (THUMBAUDIT-02). | No QRhi readback path exists; the stub fabricates a flat-color PNG. | Critical | Phase 95 | THUMBCAP-01 | Source audit proving the readback uses the frozen Option A design; Phase 98 runtime screenshot reflecting the rendered scene. |
| THUMB-MSAA-RESOLVE | N/A (no readback exists today to resolve). | Upstream renders the thumbnail at the framebuffer's sample count and resolves before producing the thumbnail image. | `RhiViewport.cpp:47` (`setSampleCount(4)` — the viewport's MSAA sample count is > 1). See Frozen Decisions for the locked resolve approach. | Freeze: render the thumbnail directly to a single-sample (sample count 1) offscreen RT so no resolve step is needed at readback (consequence of Option A). See Frozen Decisions (THUMBAUDIT-02). | The viewport is multisampled (sample count 4); readback MUST produce a non-multisampled QImage. | Critical | Phase 95 | THUMBCAP-02 | Source audit proving the offscreen RT is single-sample; Phase 98 runtime capture with no MSAA artifacts. |
| THUMB-RT-QUEUE | N/A (no capture queue exists today; the stub runs synchronously on the GUI thread). | Upstream capture is dispatched to the render context; the Qt mirror must cross the GUI→render thread boundary. | Template pattern: `RhiViewport.h:314-315` (`m_fitRequestCount`/`m_viewPreset` member fields); `RhiViewport.cpp:415,435` (increment `m_fitRequestCount` + trigger `update()`); `RhiViewport.cpp:442` (set `m_viewPreset`); `RhiViewportRenderer.cpp:35` (`synchronize(QQuickRhiItem *item)` copies item→renderer state on the render thread). See Frozen Decisions for the locked queue design. | Freeze: a capture request flag + target plateIndex/size set on `RhiViewport` (GUI thread), triggered via `update()` so `synchronize()` runs on the render thread, which copies the request to `RhiViewportRenderer`, performs the readback inside `render()`, and delivers the resulting QImage back via a queued signal/connection. See Frozen Decisions (THUMBAUDIT-02). | No render-thread capture queue or item→renderer→item callback exists; the stub runs entirely on the GUI thread. | Critical | Phase 95 | THUMBCAP-03 | Source audit proving the queue mirrors the `m_fitRequestCount`/`m_viewPreset` synchronize() pattern; Phase 98 runtime capture delivered to the GUI thread. |
| THUMB-PLATE-CACHE | The destination cache is already in place: `PartPlate.h:122-124` (`thumbnail()`/`setThumbnail(QImage)`/`hasThumbnail()`), `:255` (`QImage m_thumbnail`); invalidated on content mutation at `:116` (setLocked), `:157` (addInstance), `:163` (removeInstance). | `PartPlate::thumbnail_data` destination (upstream `PartPlate.hpp` cache that the writer reads and the loader populates). | `src/core/model/PartPlate.h:122-124,255` (`thumbnail()`/`setThumbnail(QImage)`/`hasThumbnail()`/`m_thumbnail`); invalidation at `:116,157,163`. | Preserve (the destination cache is already in place). | None for the cache itself — it is ready to receive real captured pixels from Phase 95. | Low | Phase 97 | THUMBRT-01 | Source audit confirming the cache API + invalidation hooks are unchanged; Phase 97 round-trip test asserting `PartPlate::thumbnail()` survives save→reload. |
| THUMB-CLEANUP | Removal candidates: the mock generator (`ProjectServiceMock.cpp:4242,4362,4372`) and the solid-color stub (`RhiViewport.cpp:476-488`) once the real capture path lands. | No-dead-paths rule (No Deprecated UI Rule from PROJECT.md): once the real capture path is the sole source, the mock paths must leave no dead/disconnected code. | `src/core/services/ProjectServiceMock.cpp:4242,4362,4372`; `src/qml_gui/Renderer/RhiViewport.cpp:476-488`. | Remove (mock paths) — owned by Phase 95/96 (replacement lands there); Phase 98 audits no dead/disconnected paths remain. | Mock generator + solid-color stub remain as the only thumbnail paths today; they become dead once the real path lands. | High | Phase 98 | THUMBVERIFY-01 | Source audit proving no dead/disconnected mock paths remain after Phase 95/96 replacements; Phase 98 canonical verifier + regression ctest. |

## Frozen Decisions (THUMBAUDIT-02)

The three designs below are locked before implementation. Phase 95 implements
exactly these choices; deviations require re-opening THUMBAUDIT-02.

### 1. QRhi readback approach (THUMB-RHI-READBACK) — LOCKED: Option A

Two candidate approaches were evaluated:

- **Option A — offscreen QRhiTexture render-target + `readBackTexture`.** Render
  the scene into a separate offscreen `QRhiTexture` render-target at thumbnail
  size, then call `QRhiResourceUpdateBatch::readBackTexture()` to read the
  pixels back into CPU memory. Clean separation from the on-screen render pass;
  the RT is sized exactly to the thumbnail dimensions (no downscale needed).
  Cost: the render pass is duplicated (once on-screen, once off-screen) for a
  capture frame.
- **Option B — live `renderTarget()` color attachment readback + downscale.**
  Read back the live on-screen `renderTarget()` color attachment after a render
  pass, then downscale to thumbnail size. Reuses the existing render pass (no
  duplication). Cost: the live RT is multisampled (sample count 4 per
  `RhiViewport.cpp:47 setSampleCount(4)`), so a resolve step is required before
  readback; and the RT is sized to the viewport (not thumbnail size), so a
  downscale is required.

**Upstream behavior truth:** upstream OrcaSlicer renders a dedicated thumbnail
via a separate framebuffer (a distinct render target sized to the thumbnail),
not by reading back the live on-screen framebuffer. This maps directly to
Option A's offscreen RT.

**Locked choice: Option A.** Rationale: (a) it mirrors upstream's dedicated
thumbnail framebuffer; (b) it yields a thumbnail-sized, single-sample texture
(see THUMB-MSAA-RESOLVE below) with no resolve or downscale step; (c) the
duplicated render pass runs only on capture frames (rare, triggered by an
explicit `requestThumbnailCapture` call), so the steady-state cost is zero.

**Entry point:** `RhiViewport.h:229` `requestThumbnailCapture` (Q_INVOKABLE,
GUI thread). **Readback site:** inside `RhiViewportRenderer::render()` (render
thread), where the QRhi context is current. Severity Critical; Owner Phase 95;
Requirement THUMBCAP-01.

### 2. MSAA resolve approach (THUMB-MSAA-RESOLVE) — LOCKED: render-to-single-sample offscreen RT

The viewport sets MSAA sample count 4: `RhiViewport.cpp:47 setSampleCount(4)`.
The on-screen color attachment is therefore multisampled, and any readback of
it MUST resolve the multisampled attachment before producing a
non-multisampled QImage. The two approaches:

- For **Option A** (locked readback): render the thumbnail directly into a
  single-sample (sample count 1) offscreen RT. Because the offscreen RT is not
  multisampled, no resolve step is needed at readback — `readBackTexture()`
  yields a non-multisampled result directly.
- For Option B (not chosen): the live `renderTarget()` color attachment is
  multisampled, so a resolve step (resolve-to-texture, or reliance on
  QQuickRhiItem's built-in resolve) would be required before readback.

**Locked choice:** as a direct consequence of locking Option A, the thumbnail
is rendered to a single-sample offscreen RT (sample count 1), so the resolve
step is eliminated entirely. This avoids both the resolve complexity and any
MSAA readback artifacts. The on-screen viewport keeps its sample count 4
(`RhiViewport.cpp:47`) for steady-state rendering quality; only the offscreen
thumbnail RT is single-sample. Severity Critical; Owner Phase 95; Requirement
THUMBCAP-02.

### 3. Render-thread capture queue + QImage callback (THUMB-RT-QUEUE) — LOCKED: mirror the `m_fitRequestCount`/`m_viewPreset` synchronize() pattern

`requestThumbnailCapture` lives on `RhiViewport` (the QML item, GUI thread) but
the QRhi readback MUST happen inside `RhiViewportRenderer::render()` (render
thread), because the QRhi context is only current on the render thread. The
request therefore must cross the GUI→render thread boundary, and the resulting
QImage must cross back.

The existing `m_fitRequestCount`/`m_viewPreset` pattern is the template:

- `RhiViewport.h:314-315` — the `m_fitRequestCount` and `m_viewPreset` member
  fields on the item.
- `RhiViewport.cpp:415,435` — the item increments `m_fitRequestCount` and calls
  `update()` to schedule a render pass (which drives `synchronize()`).
- `RhiViewport.cpp:442` — the item sets `m_viewPreset` (a request field).
- `RhiViewportRenderer.cpp:35` — `synchronize(QQuickRhiItem *item)` copies
  item→renderer state on the render thread before `render()` runs.

**Locked design:** a capture request flag + target `plateIndex`/`size` set on
`RhiViewport` (GUI thread) inside `requestThumbnailCapture`, then `update()` is
called so `synchronize()` runs on the render thread. `synchronize()` copies the
capture request (flag + plateIndex + size) to `RhiViewportRenderer`. Inside
`render()`, the renderer performs the Option A offscreen render + readback, then
delivers the resulting QImage back to the item/GUI thread via a queued
signal/connection (mirroring the item→renderer→item flow that the
`thumbnailCaptured` signal at `RhiViewport.h:239` already advertises). This
keeps all GPU work on the render thread and all QImage consumption on the GUI
thread, matching the existing two-thread discipline. Severity Critical; Owner
Phase 95; Requirement THUMBCAP-03.

## v4.3 Requirement Routing

Every v4.3 requirement is routed to its owner phase and matrix region(s).

| Requirement | Owner | Matrix Region(s) |
|---|---|---|
| THUMBAUDIT-01 | Phase 94 | All canonical regions in this matrix (THUMB-MOCK-GENERATOR through THUMB-CLEANUP). |
| THUMBAUDIT-02 | Phase 94 | Frozen Decisions section above (THUMB-RHI-READBACK Option A; THUMB-MSAA-RESOLVE single-sample offscreen RT; THUMB-RT-QUEUE synchronize() queue pattern). |
| THUMBCAP-01 | Phase 95 | THUMB-REQUEST-STUB, THUMB-RHI-READBACK. |
| THUMBCAP-02 | Phase 95 | THUMB-MSAA-RESOLVE. |
| THUMBCAP-03 | Phase 95 | THUMB-RT-QUEUE. |
| THUMBWRITE-01 | Phase 96 | THUMB-WRITE-PLATE. |
| THUMBWRITE-02 | Phase 96 | THUMB-WRITE-STOREPARAMS. |
| THUMBWRITE-03 | Phase 96 | THUMB-WRITE-PLATE, THUMB-WRITE-STOREPARAMS (the `_add_thumbnail_file_to_archive` PNG encoding path that runs to completion). |
| THUMBRT-01 | Phase 97 | THUMB-READ-SIDE, THUMB-PLATE-CACHE. |
| THUMBRT-02 | Phase 97 | THUMB-READ-SIDE (the automated round-trip test asserting pixels survive save→reload). |
| THUMBVERIFY-01 | Phase 98 | THUMB-CLEANUP plus source audits for THUMB-MOCK-GENERATOR and THUMB-REQUEST-STUB. |
| THUMBVERIFY-02 | Phase 98 | Final runtime launch, canonical verifier, Prepare/Preview/AssembleView regression ctest, and runtime thumbnail capture reachability. |

## Out-of-Scope Classification

The following items are explicitly out of scope for v4.3. No phase in v4.3
re-touches them unless the user explicitly reopens them.

| Item | Status | Evidence / Reason |
|---|---|---|
| Auto filament-map recommendation + wipe-tower geometry/rendering | Out of scope — future milestone | Separate algorithm + UI milestone; depends on the PartPlate model. v4.3 is thumbnail capture + 3MF round-trip only. |
| Missing CLI fixtures + deterministic argv-based GUI fixture loading for screenshots | Out of scope — future fixture milestone | FIXTURE-02 is unblocked by v4.3 closing the shared writer blocker, but the fixtures themselves are a future milestone. |
| D3D12 or Vulkan backend promotion | Out of scope — future backend work | D3D12 has a known startup crash and remains explicit opt-in; Vulkan is disabled in the current Qt 6.10 SDK. Thumbnail capture runs on the default RHI/D3D11 path. |
| Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper | Out of scope — future milestone | Needs per-volume ITS + scene raycaster. Not required for thumbnail capture. |
| LAN/device/cloud/network/Monitor/ModelMall/Home WebView/camera/printer-hardware workflows | Out of scope — removed scope | Removed from forward scope by user direction on 2026-07-07. The v4.3 milestone is local/offline thumbnail capture + 3MF persistence only. Not reintroduced unless the user explicitly reopens it. |
| libslic3r slicing algorithm changes | Out of scope — engine preserved | The migration rewrites the GUI layer only; libslic3r slicing algorithms are preserved unchanged. Thumbnail capture must not change slicing engine behavior. |

## Requirement Coverage

| Requirement | Covered By |
|---|---|
| THUMBAUDIT-01 | This matrix maps all 10 thumbnail-capture regions (THUMB-MOCK-GENERATOR through THUMB-CLEANUP) to OrcaSlicer upstream source files (with line anchors: `bbs_3mf.cpp:6133,6137,5879,1640`, `PartPlate::store_to_3mf_structure`), current Qt mock paths (`ProjectServiceMock.cpp:4242,4362,4372,5089-5093,5127-5135,5455-5466,5654-5660`; `RhiViewport.cpp:476-488`; `RhiViewport.h:229`; `PartPlate.h:122-124,255`), the QRhi readback approach (Frozen Decisions Option A), the MSAA resolve strategy (single-sample offscreen RT), and verification expectations (Phase 97 round-trip + Phase 98 canonical verifier + visual evidence). |
| THUMBAUDIT-02 | The Frozen Decisions section locks three designs before implementation: (1) QRhi readback approach = Option A (offscreen `QRhiTexture` render-target at thumbnail size + `QRhiResourceUpdateBatch::readBackTexture()`), with the rationale that it mirrors upstream's dedicated thumbnail framebuffer and yields a thumbnail-sized single-sample texture; (2) MSAA resolve approach = render the thumbnail to a single-sample (sample count 1) offscreen RT so no resolve step is needed at readback (citing `RhiViewport.cpp:47 setSampleCount(4)`); (3) render-thread capture request queue + QImage callback = mirror the existing `m_fitRequestCount`/`m_viewPreset` synchronize() pattern (`RhiViewport.h:314-315`, `RhiViewport.cpp:415,435,442`, `RhiViewportRenderer.cpp:35`), with the readback inside `render()` and the QImage delivered back via a queued signal/connection. |

## Phase Routing

| Phase | Work To Start From This Audit |
|---|---|
| 95 | Implement the real QRhi texture readback capture using the frozen Option A design (THUMB-RHI-READBACK, THUMB-REQUEST-STUB); render the thumbnail to a single-sample offscreen RT (THUMB-MSAA-RESOLVE); wire the render-thread capture queue + QImage callback mirroring the `m_fitRequestCount`/`m_viewPreset` synchronize() pattern (THUMB-RT-QUEUE). |
| 96 | Populate `PlateData::plate_thumbnail` in `buildPlateDataList` (THUMB-WRITE-PLATE); populate `StoreParams::thumbnail_data` in `saveProject` (THUMB-WRITE-STOREPARAMS); make the upstream `store_bbs_3mf` PNG encoding path (`_add_thumbnail_file_to_archive`) run to completion on the Qt6 pipeline (THUMBWRITE-03). |
| 97 | Verify the existing read side restores saved thumbnails (THUMB-READ-SIDE, THUMB-PLATE-CACHE); add the automated round-trip test asserting pixels survive save→reload (THUMBRT-02). |
| 98 | Remove dead mock thumbnail paths (THUMB-CLEANUP); run the canonical verifier, launch the app, confirm Prepare/Preview/AssembleView regression-free, and record runtime thumbnail capture evidence (THUMBVERIFY-01, THUMBVERIFY-02). |
