---
gsd_state_version: 1.0
milestone: v4.3
milestone_name: Real Thumbnail Capture And 3MF Round-Trip
status: executing
last_updated: 2026-07-10T08:42:26+08:00
last_activity: 2026-07-10 -- Phase 95 plan 01 complete (real QRhi thumbnail capture infrastructure)
progress:
  total_phases: 5
  completed_phases: 1
  total_plans: 2
  completed_plans: 2
  percent: 40
stopped_at: Phase 95 plan 01 complete; ready to plan Phase 96
---

# Project State

**Milestone:** v4.3 - Real Thumbnail Capture And 3MF Round-Trip
**Status:** Executing (Phase 95 capture infrastructure complete)
**Next step:** Plan Phase 96 with `/gsd-plan-phase 96`.

## Current Position

Phase: 95 (QRhi Thumbnail Capture Infrastructure) — complete
Plan: 01/01 complete
Status: Phase 95 capture infrastructure shipped; ready to plan Phase 96
Last activity: 2026-07-10 — Phase 95 plan 01 executed (real QRhi offscreen RT readback replacing solid-color stub; commit fc4aadb)

## Current Milestone (v4.3)

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 94 | Thumbnail Capture Gap Audit | Complete | THUMBAUDIT-01, THUMBAUDIT-02 |
| 95 | QRhi Thumbnail Capture Infrastructure | Complete | THUMBCAP-01, THUMBCAP-02, THUMBCAP-03 |
| 96 | 3MF Thumbnail Write Integration | Not started | THUMBWRITE-01, THUMBWRITE-02, THUMBWRITE-03 |
| 97 | Thumbnail Save-Reload Round-Trip | Not started | THUMBRT-01, THUMBRT-02 |
| 98 | Thumbnail Verification And Cleanup | Not started | THUMBVERIFY-01, THUMBVERIFY-02 |

## Phase 95 Frozen Decisions Implemented (THUMBCAP-01/02/03)

- **Real QRhi readback (THUMBCAP-01):** Offscreen single-sample `QRhiTexture` render-target at thumbnail size + `QRhiResourceUpdateBatch::readBackTexture()` with its own `QRhiRenderPassDescriptor`; offscreen pass reuses the on-screen scene vertex buffers (bed fill + grid lines + model mesh) with a thumbnail-aspect (1.0) camera MVP. Stub `#18222c` PNG fabrication removed from `requestThumbnailCapture`.
- **Single-sample RT no MSAA resolve (THUMBCAP-02):** Thumbnail RT is sample count 1, so no resolve step is needed at readback. On-screen viewport keeps sample count 4.
- **Render-thread capture queue + queued callback (THUMBCAP-03):** Item-side `m_thumbnailRequestPending`/`m_thumbnailPlateIndex`/`m_thumbnailSize` mirrored into the renderer via `synchronize()`; async readback polled at the start of the next `render()` frame; `QImage` delivered back to `RhiViewport::deliverThumbnail` on the GUI thread via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`. `QPointer<RhiViewport>` survives item deletion.

## Phase 94 Frozen Decisions (THUMBAUDIT-02)

- **QRhi readback (THUMB-RHI-READBACK):** Option A — offscreen `QRhiTexture` render-target at thumbnail size + `QRhiResourceUpdateBatch::readBackTexture()` (mirrors upstream's dedicated thumbnail framebuffer; yields a single-sample thumbnail-sized texture). Drives Phase 95 THUMBCAP-01.
- **MSAA resolve (THUMB-MSAA-RESOLVE):** render the thumbnail to a single-sample (sample count 1) offscreen RT so no resolve step is needed at readback (consequence of Option A); on-screen viewport keeps sample count 4 per `RhiViewport.cpp:47`. Drives Phase 95 THUMBCAP-02.
- **Render-thread capture queue (THUMB-RT-QUEUE):** mirror the existing `m_fitRequestCount`/`m_viewPreset` synchronize() pattern (`RhiViewport.h:314-315`, `RhiViewport.cpp:415,435,442`, `RhiViewportRenderer.cpp:35`); readback inside `render()`, QImage delivered back via queued signal/connection. Drives Phase 95 THUMBCAP-03.

## Last Completed Milestone: v4.2

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 89 | AssembleView Source-Truth Gap Audit | Complete | ASMAUDIT-01, ASMAUDIT-02 |
| 90 | AssembleView Shell And Canvas Host Restoration | Complete | ASMSHELL-01, ASMSHELL-02, ASMROUTE-01 |
| 91 | Explosion Ratio And Assembly Rendering | Complete | ASMEXPLODE-01, ASMEXPLODE-02 |
| 92 | Assembly Measurement Gizmo | Complete | ASMMEASURE-01, ASMMEASURE-02 |
| 93 | AssembleView Verification And Cleanup | Complete | ASMROUTE-02, ASMVERIFY-01, ASMVERIFY-02 |

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-07-10)

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** v4.3 — real QRhi thumbnail capture + 3MF write-side persistence + save→reload round-trip, closing v3.2 THUMB-02/THUMB-03.

## Milestone Context (v4.3)

**Goal:** Replace mock thumbnails with real QRhi framebuffer capture, and close the 3MF write side so thumbnails survive save → reload.

**Current state (from pre-planning exploration):**
- Mock thumbnails: `ProjectServiceMock.cpp:4242 generatePlateThumbnail()` draws QPainter flat-color placeholders; `RhiViewport.cpp:476 requestThumbnailCapture()` is a solid-color PNG stub. Zero real readback code in repo.
- 3MF write side explicitly NOT populated: `saveProject` (`ProjectServiceMock.cpp:5100`) omits `StoreParams::thumbnail_data` and `PlateData::plate_thumbnail` (comments at `:5089-5093` and `:5127-5135`) because the upstream writer's PNG encoding path throws on the Qt6 mock pipeline.
- 3MF READ side fully implemented: `ProjectServiceMock.cpp:5455-5466` extracts `plate->plate_thumbnail` → QImage, applied at `5654-5660`. Only the write side is missing.
- Infrastructure present: `RhiViewport : QQuickRhiItem` exposes `rhi()`/`renderTarget()`; `RhiViewportRenderer : QQuickRhiItemRenderer`. Need: render-thread capture queue + `QRhiReadback`/`QRhiReadbackResult` + MSAA resolve (sample count > 1 per `RhiViewport.cpp:43`).

**v3.2 deferred items being closed:**
- THUMB-02 (partial v3.2): write `PlateData::plate_thumbnail` pixels into 3MF so save→reload round-trips. Blocked because writer PNG path throws on mock pixels.
- THUMB-03 (deferred v3.3+): real GL/QRhi-capture thumbnails. Unblocks both THUMB-02 and the shared `store_bbs_3mf` writer blocker (also blocks FIXTURE-02).

**Out of scope for v4.3:**
- Auto filament-map + wipe-tower.
- CLI fixtures + deterministic screenshots.
- D3D12 root cause.
- Full GLGizmoMeasure engine + clipper.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | v4.2 AssembleView source-truth restoration | Shipped in v4.2 |
| active | Real thumbnail capture + 3MF round-trip | v4.3 |
| removed | LAN/device/cloud/network/Monitor workflows | Removed from future scope by user direction on 2026-07-07 |
| future | Auto filament-map recommendation + wipe-tower geometry/rendering | Future milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone (FIXTURE-02 unblocked by v4.3) |
| future | D3D12 root cause | Dedicated backend investigation milestone |
| future | Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper | Future milestone (needs per-volume ITS) |

## Deferred Items

Items acknowledged and deferred at v4.2 milestone close on 2026-07-09:

| Category | Item | Status |
|---|---|---|
| process | Nyquist VALIDATION.md files | Phases 89-93 have deterministic verification artifacts but no separate Nyquist validation files |
| evidence | Runtime visual evidence | Windows capture API blocked; reachability via process-liveness + canonical verifier + regression ctest |
| build | Canonical build libslic3r reconfigure | Per-invocation ~8 min reconfigure timed out executor wrapper in code phases; production code clean |
| feature | Full GLGizmoMeasure engine + clipper | Deferred (needs per-volume ITS + raycaster) |

## Scope Guard

- v4.3 is local/offline thumbnail capture + 3MF persistence work only.
- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.

## Operator Next Steps

- Define REQUIREMENTS.md for v4.3.
- Create ROADMAP.md (phases continue from 94).
