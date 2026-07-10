# Requirements: OWzx Slicer v4.3 Real Thumbnail Capture And 3MF Round-Trip

**Defined:** 2026-07-10
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v4.3 Requirements

### Source-Truth Inventory

- [x] **THUMBAUDIT-01**: The thumbnail capture + 3MF writer surface has a current inventory mapping the mock paths (`generatePlateThumbnail` QPainter stub, `requestThumbnailCapture` solid-color stub), the deferred write-side gaps, upstream source anchors (`store_bbs_3mf` thumbnail path, `PartPlate::store_to_3mf_structure`), the QRhi readback approach, MSAA resolve strategy, and verification expectations.
- [x] **THUMBAUDIT-02**: The QRhi readback design (offscreen `QRhiTexture` render-target + `readBackTexture`, vs live RT color attachment readback), the MSAA resolve approach (sample count > 1), and the render-thread capture request queue + QImage callback design are frozen as locked decisions before implementation.

### RHI Capture Infrastructure

- [x] **THUMBCAP-01**: The Qt6 app captures a real screenshot of the current 3D viewport via QRhi texture readback, replacing the `RhiViewport::requestThumbnailCapture` solid-color stub (`RhiViewport.cpp:476`) so the captured pixels reflect the actual rendered scene (bed, plate, model mesh, gizmos).
- [x] **THUMBCAP-02**: The capture handles multisampled (MSAA) render targets — the viewport sets sample count > 1 (`RhiViewport.cpp:43`) — so the readback resolves the multisampled color attachment before producing a non-multisampled QImage.
- [x] **THUMBCAP-03**: The capture runs on the render thread (inside `RhiViewportRenderer::render`) via a cross-thread request queue (item → renderer), and delivers the resulting QImage back to the item/GUI thread via a callback, mirroring the existing `m_fitRequestCount`/`m_viewPreset` request pattern.

### 3MF Write Integration

- [ ] **THUMBWRITE-01**: On project save, `ProjectServiceMock::saveProject` populates `PlateData::plate_thumbnail` (in `buildPlateDataList`) with real captured pixels, so per-plate thumbnails are written into the 3MF archive (closes the v3.2 THUMB-02 write-side gap; comments at `ProjectServiceMock.cpp:5089-5093`).
- [ ] **THUMBWRITE-02**: On project save, `StoreParams::thumbnail_data` is populated with a real captured thumbnail so the project-level thumbnail is written into the 3MF archive (closes the `ProjectServiceMock.cpp:5127-5135` deferred write).
- [ ] **THUMBWRITE-03**: The upstream `store_bbs_3mf` PNG encoding path (`_add_thumbnail_file_to_archive`) runs to completion on the Qt6 pipeline without throwing the non-std exception previously seen on the mock pipeline.

### Round-Trip Verification

- [ ] **THUMBRT-01**: Saving a project with captured thumbnails and reloading it restores the thumbnails via the existing read side (`ProjectServiceMock.cpp:5455-5466`) so the reloaded `PartPlate::thumbnail()` matches the saved pixels (THUMB-02 closure).
- [ ] **THUMBRT-02**: The round-trip is covered by an automated test (PartPlateTests or a new round-trip test) that saves a project with a known thumbnail, reloads it, and asserts the thumbnail pixels survive.

### Cleanup And Regression

- [ ] **THUMBVERIFY-01**: Replaced mock thumbnail code (QPainter placeholder generator + solid-color stub) leaves no dead/disconnected paths; the real capture path is the sole source.
- [ ] **THUMBVERIFY-02**: The canonical verifier passes, `build/OWzxSlicer.exe` launches, Prepare/Preview/AssembleView rendering is regression-free (PrepareSceneDataTests, ViewModelSmokeTests, QmlUiAuditTests pass), and runtime thumbnail capture is reachable.

## Future Requirements

### Adjacent Local/Offline Work

- **AUTO-FILAMENT-FUTURE-01**: Auto filament-map recommendation and wipe-tower geometry/rendering as a dedicated milestone.
- **FIXTURE-FUTURE-01**: Add missing CLI fixtures (`hotend.stl`, `Block20XY.stl`) and deterministic GUI fixture loading for visual screenshots (unblocked by v4.3 closing the shared writer blocker).
- **BACKEND-FUTURE-01**: Resolve the D3D12 QRhi crash and evaluate Vulkan only after an SDK/runtime path exists.
- **MEASURE-FUTURE-01**: Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper (needs per-volume ITS).

### Removed Product Scope

- **NETWORK-REMOVED-01**: LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows are not future requirements unless the user explicitly reopens them.

## Out of Scope

Explicitly excluded to keep v4.3 focused.

| Feature | Reason |
|---|---|
| Auto filament-map recommendation and wipe-tower geometry/rendering | Separate algorithm+UI milestone; depends on PartPlate model. |
| D3D12 or Vulkan backend promotion | Renderer backend work is blocked/future and not required for thumbnail capture on D3D11. |
| libslic3r slicing algorithm changes | Thumbnail capture must not change slicing engine behavior. |
| Live camera/network/device workflows | Removed from forward product scope by user direction on 2026-07-07. |
| New product behavior not mapped to OrcaSlicer upstream | Violates the project core value. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| THUMBAUDIT-01 | Phase 94 | Complete |
| THUMBAUDIT-02 | Phase 94 | Complete |
| THUMBCAP-01 | Phase 95 | Complete |
| THUMBCAP-02 | Phase 95 | Complete |
| THUMBCAP-03 | Phase 95 | Complete |
| THUMBWRITE-01 | Phase 96 | Not started |
| THUMBWRITE-02 | Phase 96 | Not started |
| THUMBWRITE-03 | Phase 96 | Not started |
| THUMBRT-01 | Phase 97 | Not started |
| THUMBRT-02 | Phase 97 | Not started |
| THUMBVERIFY-01 | Phase 98 | Not started |
| THUMBVERIFY-02 | Phase 98 | Not started |

**Coverage:**
- v4.3 requirements: 12 total
- Mapped to phases: 12
- Unmapped: 0

---
*Requirements defined: 2026-07-10*
*Last updated: 2026-07-10 after Phase 95 plan 01 complete (THUMBCAP-01/02/03 marked complete)*
