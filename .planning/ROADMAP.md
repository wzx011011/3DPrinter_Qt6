# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- 🚧 **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (in progress)

## Current Milestone: v4.3 Real Thumbnail Capture And 3MF Round-Trip

**Goal:** Replace mock thumbnails with real QRhi framebuffer capture, and close the 3MF write side so thumbnails survive a save → reload round-trip — closing the v3.2 THUMB-02/THUMB-03 deferred items and unblocking FIXTURE-02's shared writer blocker.

**Scope rule:** Local/offline only. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from scope.

**Current state (from pre-planning exploration):**
- Mock thumbnails: `ProjectServiceMock.cpp:4242 generatePlateThumbnail()` (QPainter flat-color placeholders); `RhiViewport.cpp:476 requestThumbnailCapture()` (solid-color PNG stub). Zero readback code in repo.
- 3MF write side explicitly NOT populated (`saveProject` omits `thumbnail_data`/`plate_thumbnail`); read side fully implemented (`ProjectServiceMock.cpp:5455-5466`).
- Infrastructure present: `RhiViewport : QQuickRhiItem`, `RhiViewportRenderer : QQuickRhiItemRenderer` expose `rhi()`/`renderTarget()`. Need: render-thread capture queue + `QRhiReadback` + MSAA resolve.

## Phases

- [x] Phase 94: Thumbnail Capture Gap Audit
- [ ] Phase 95: QRhi Thumbnail Capture Infrastructure
- [ ] Phase 96: 3MF Thumbnail Write Integration
- [ ] Phase 97: Thumbnail Save-Reload Round-Trip
- [ ] Phase 98: Thumbnail Verification And Cleanup

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 94 | Thumbnail Capture Gap Audit | Freeze the v4.3 capture+writer region map: current mock paths, upstream writer anchors, Qt RHI readback approach, MSAA handling, replacement decisions, and verification expectations before edits. | THUMBAUDIT-01, THUMBAUDIT-02 |
| 95 | QRhi Thumbnail Capture Infrastructure | Implement real QRhi texture readback capture replacing the `requestThumbnailCapture` stub, with MSAA resolve and a render-thread capture queue + QImage callback. | THUMBCAP-01, THUMBCAP-02, THUMBCAP-03 |
| 96 | 3MF Thumbnail Write Integration | Populate `PlateData::plate_thumbnail` + `StoreParams::thumbnail_data` on save, and make the upstream `store_bbs_3mf` PNG encoding path run to completion on the Qt6 pipeline. | THUMBWRITE-01, THUMBWRITE-02, THUMBWRITE-03 |
| 97 | Thumbnail Save-Reload Round-Trip | Verify the existing read side restores saved thumbnails and add an automated round-trip test asserting pixels survive save → reload (THUMB-02 closure). | THUMBRT-01, THUMBRT-02 |
| 98 | Thumbnail Verification And Cleanup | Remove dead mock thumbnail paths, run canonical verifier, confirm Prepare/Preview/AssembleView regression-free, and record runtime capture evidence. | THUMBVERIFY-01, THUMBVERIFY-02 |

### Phase 94: Thumbnail Capture Gap Audit

**Status:** Complete
**Plans:** 1/1

Success criteria:
1. The thumbnail capture + 3MF writer surface is mapped to upstream source anchors (`store_bbs_3mf` thumbnail path, `PartPlate::store_to_3mf_structure`), current Qt mock paths, replacement decisions, and verification expectations.
2. The QRhi readback approach (offscreen QRhiTexture RT render + `readBackTexture`, or live RT color attachment readback), MSAA resolve strategy, and render-thread request queue design are frozen before implementation.

### Phase 95: QRhi Thumbnail Capture Infrastructure

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. `RhiViewport::requestThumbnailCapture` produces a real screenshot reflecting the rendered scene (bed/plate/mesh/gizmos), replacing the solid-color stub.
2. The capture handles the MSAA render target (sample count > 1) — readback resolves the multisampled color attachment to a non-multisampled QImage.
3. The capture runs on the render thread via a request queue (item → renderer) and delivers the QImage back to the GUI thread via callback (mirroring the `m_fitRequestCount`/`m_viewPreset` pattern).

### Phase 96: 3MF Thumbnail Write Integration

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. `buildPlateDataList` populates `PlateData::plate_thumbnail` with real captured pixels on save.
2. `saveProject` populates `StoreParams::thumbnail_data` with a real captured project thumbnail.
3. The upstream `store_bbs_3mf` PNG encoding path (`_add_thumbnail_file_to_archive`) runs to completion without throwing the non-std exception seen on the mock pipeline.

### Phase 97: Thumbnail Save-Reload Round-Trip

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. Saving a project with captured thumbnails and reloading it restores the thumbnails via the existing read side so the reloaded `PartPlate::thumbnail()` matches the saved pixels.
2. An automated test (PartPlateTests or a new round-trip test) saves a project with a known thumbnail, reloads it, and asserts the thumbnail pixels survive.

### Phase 98: Thumbnail Verification And Cleanup

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. Replaced mock thumbnail code (QPainter placeholder generator + solid-color stub) leaves no dead/disconnected paths; the real capture path is the sole source.
2. The canonical verifier passes, `build/OWzxSlicer.exe` launches, Prepare/Preview/AssembleView rendering is regression-free, and runtime thumbnail capture is reachable.

## Deferred Backlog

- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Missing CLI fixtures and deterministic argv-based GUI fixture loading for screenshots (FIXTURE-02 unblocked by v4.3).
- D3D12 root-cause investigation and future Vulkan/D3D12 backend promotion.
- Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper (needs per-volume ITS).

## Removed Scope

- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware calibration are no longer backlog items.

## Next Step

Plan Phase 94 after this roadmap is approved:

```text
$gsd-plan-phase 94
```

---

*Last updated: 2026-07-10 at v4.3 milestone start.*
