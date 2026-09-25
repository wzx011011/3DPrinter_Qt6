# Phase 98: Thumbnail Verification And Cleanup - Context

**Gathered:** 2026-07-10
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Replaced mock thumbnail code (QPainter placeholder generator + solid-color stub) leaves no dead/disconnected paths; the real capture path is the sole source. The canonical verifier passes, `build/OWzxSlicer.exe` launches, Prepare/Preview/AssembleView rendering is regression-free, and runtime thumbnail capture is reachable.

Success criteria (from ROADMAP):
1. THUMBVERIFY-01: Mock thumbnail code removed cleanly (no dead paths); real QRhi capture is the sole source.
2. THUMBVERIFY-02: Canonical verifier passes, OWzxSlicer.exe launches, Prepare/Preview/AssembleView regression-free, runtime capture reachable.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user setting. Use ROADMAP phase goal, success criteria, codebase conventions, and the Phase 94-97 frozen decisions to guide choices.

### Carry-Forward Inputs (frozen in prior phases)
- Phase 94 (THUMBAUDIT-02): QRhi offscreen readback approach decided.
- Phase 95 (THUMBCAP-01/02/03): Real QRhi capture infrastructure shipped — `RhiViewport::requestThumbnailCapture` drives offscreen RT + `readBackTexture()` + queued callback to `deliverThumbnail`.
- Phase 96 (THUMBWRITE-01/02/03): 3MF write side closed — `qimageToThumbnailData` helper, `PlateData::plate_thumbnail` populate, `StoreParams::thumbnail_data` populate.
- Phase 97 (THUMBRT-01/02): Save→reload round-trip verified end-to-end via `thumbnailSaveReloadRoundTrip` ctest slot. Read-side extraction helper `extractPlateThumbnailFrom3mf` added. Ordering fix: thumbnails restored AFTER `arrangeObjects` (commit `efd5e42`).

### Known Phase 98 Scope (from Phase 97 code review + CONTEXT carry-forward)
1. **Mock generator removal (THUMBVERIFY-01):** Remove `ProjectServiceMock.cpp:~4242 generatePlateThumbnail()` QPainter placeholder and `generatePlateThumbnailVariant`. Audit `requestThumbnailCapture` dead-path. The real QRhi capture path (Phase 95) is the sole source.
2. **Canonical verifier pass (THUMBVERIFY-02):** Run `scripts/auto_verify_with_vcvars.ps1` end-to-end. Confirm `OWzxSlicer.exe` launches, Prepare/Preview/AssembleView regression-free.
3. **Multi-plate thumbnail gap (MEDIUM from Phase 97 code review):** The write side only archives ONE plate's PNG bytes (current plate) but emits per-plate XML refs. Plates > 0 get XML ref with no PNG bytes. Phase 98 should add a multi-plate round-trip test + fix the write side to push one `thumbnail_data` entry per plate.
4. **Optional harness cleanup:** Scratch build logs (`build_phase*.log`, `build_tests*.log`, `build_ppt*.log`, `build_probe.log`, `build_restore.log`) and scratch scripts (`scripts/run_partplate_only.ps1`, `scripts/build_run_ppt.ps1`) accumulated across Phases 95-97 — remove if not referenced.

</decisions>

<code_context>
## Existing Code Insights

Codebase context will be gathered during plan-phase research. Key anchors:
- `src/core/services/ProjectServiceMock.cpp:~4242 generatePlateThumbnail()` (mock generator removal target)
- `src/core/services/ProjectServiceMock.cpp:~4290 generatePlateThumbnailVariant()` (variant mock)
- `src/qml_gui/Renderer/RhiViewport.cpp:~476 requestThumbnailCapture()` (Phase 95 real capture — sole source after cleanup)
- `src/core/services/ProjectServiceMock.cpp:5143 extractPlateThumbnailFrom3mf()` (Phase 97 read helper)
- `src/core/services/ProjectServiceMock.cpp:~5290 saveProject` StoreParams::thumbnail_data populate (Phase 96 write site — multi-plate gap)

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

Per ROADMAP "Deferred Backlog" + "Removed Scope" sections — do NOT promote any of these:
- Auto filament-map recommendation + wipe-tower.
- CLI fixtures + deterministic argv GUI fixture loading (FIXTURE-02).
- D3D12 root cause + Vulkan/D3D12 backend.
- Full GLGizmoMeasure engine + AssembleViewDataPool clipper.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

</deferred>
