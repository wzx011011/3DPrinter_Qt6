---
phase: 95-qrhi-thumbnail-capture-infrastructure
plan: 01
subsystem: rendering
tags: [qrhi, thumbnail, readback, offscreen-render-target, render-thread-queue, d3d11]

# Dependency graph
requires:
  - phase: 94-thumbnail-capture-gap-audit
    provides: Three frozen THUMBAUDIT-02 decisions (Option A offscreen RT, single-sample RT no resolve, synchronize() render-thread queue)
provides:
  - Real QRhi texture readback thumbnail capture replacing the requestThumbnailCapture solid-color stub
  - Offscreen single-sample QRhiTexture render-target at thumbnail size with its own render-pass descriptor
  - Async readBackTexture + next-frame poll + queued QImage callback to the GUI thread
  - Render-thread capture request queue mirroring the m_fitRequestCount/synchronize() pattern
  - Preserved public contract (lastThumbnailData Q_PROPERTY / thumbnailCaptured signal / requestThumbnailCapture Q_INVOKABLE)
affects: [96-3mf-thumbnail-write-integration, 97-thumbnail-save-reload-round-trip, 98-thumbnail-verification-and-cleanup]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Offscreen QRhiTexture render-target + readBackTexture (Option A) for thumbnail capture"
    - "Single-sample offscreen RT eliminates MSAA resolve at readback"
    - "Render-thread request queue mirroring synchronize() + queued QMetaObject::invokeMethod callback"
    - "Async readback polled at the start of the next render() frame"

key-files:
  created: []
  modified:
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - tests/QmlUiAuditTests.cpp

key-decisions:
  - "Option A offscreen QRhiTexture RT + readBackTexture (frozen decision 1) — mirrors upstream's dedicated thumbnail framebuffer"
  - "Single-sample offscreen RT so no MSAA resolve is needed at readback (frozen decision 2)"
  - "Render-thread queue mirrors the existing m_fitRequestCount/m_viewPreset synchronize() pattern + queued QImage callback (frozen decision 3)"
  - "Thumbnail pipelines are separate instances from on-screen m_fillPipeline/m_linePipeline because the render-pass descriptors differ"
  - "deliverThumbnail encodes the QImage to the same base64 PNG m_lastThumbnailData format, preserving the PreparePage.qml contract"

patterns-established:
  - "Offscreen render pass: second beginPass/endPass pair on the same command buffer after the on-screen endPass, reusing scene vertex buffers"
  - "Async readback poll: check m_thumbnailReadbackResult.data at the start of render() before the on-screen pass"
  - "Follow-up queued update() guarantees the polling frame runs even on an idle scene"

requirements-completed: [THUMBCAP-01, THUMBCAP-02, THUMBCAP-03]

# Metrics
duration: 26 min
completed: 2026-07-10
---

# Phase 95 Plan 01: QRhi Thumbnail Capture Infrastructure Summary

**Real QRhi texture readback thumbnail capture via a single-sample offscreen render-target + render-thread queue, replacing the solid-color stub**

## Performance

- **Duration:** 26 min
- **Started:** 2026-07-10T08:16:27Z
- **Completed:** 2026-07-10T08:42:26Z
- **Tasks:** 4
- **Files modified:** 5

## Accomplishments
- Removed the `requestThumbnailCapture` solid-color `#18222c` PNG stub; it now sets render-thread request fields + `update()` (no pixel fabrication on the GUI thread).
- Built a single-sample offscreen `QRhiTexture` render-target at thumbnail size with its own `QRhiRenderPassDescriptor`, and separate thumbnail fill/line pipelines bound to that RPD (not reused from the on-screen RT).
- Wired the offscreen pass to render the same scene (bed fill + grid lines + model mesh) as the on-screen View3D/AssembleView pass, reusing the existing vertex buffers (single scene-render code path parameterized by RT).
- Implemented async `readBackTexture` with a next-frame poll at the start of `render()` and a queued `QMetaObject::invokeMethod` callback delivering the `QImage` to `RhiViewport::deliverThumbnail` on the GUI thread.
- Preserved the public contract: `lastThumbnailData` Q_PROPERTY, `thumbnailCaptured` signal, and `requestThumbnailCapture` Q_INVOKABLE signature are unchanged; `deliverThumbnail` produces the same base64 PNG `m_lastThumbnailData` format so PreparePage.qml needs no change.
- Added the `rhiViewportThumbnailCaptureUsesRealReadback` audit slot (source-level, no GPU dependency) asserting the stub is gone and the real readback is wired.

## Task Commits

Each task was committed atomically. The 4 tasks landed in one cohesive commit because they are tightly interdependent (the renderer cannot compile without the item-side fields; the test references both the renderer and the item):

1. **Task 95-01-01 + 95-01-02 + 95-01-03 + 95-01-04** - `fc4aadb` (feat)

_Note: The plan noted the three implementation tasks land in one commit wave; the audit test (task 4) was included in the same commit because it gates the same source surface._

## Files Created/Modified
- `src/qml_gui/Renderer/RhiViewportRenderer.h` - Offscreen thumbnail RT members, async-readback state, `QPointer<RhiViewport>`, helper method declarations (ensureThumbnailRenderTarget/releaseThumbnailResources/renderThumbnailPass/issueThumbnailReadback/deliverCompletedThumbnail).
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` - Offscreen RT (lazy)create + release; offscene pass reusing scene buffers + thumbnail-aspect (1.0) MVP; async readback issuance + next-frame poll; queued QImage callback; render-thread request mirror in synchronize() with item-side flag clearing.
- `src/qml_gui/Renderer/RhiViewport.h` - Item-side request fields (m_thumbnailRequestPending/PlateIndex/Size) + `deliverThumbnail` declaration.
- `src/qml_gui/Renderer/RhiViewport.cpp` - `requestThumbnailCapture` now dispatches to the render thread (stub gone); `deliverThumbnail` encodes the QImage to base64 PNG + emits thumbnailCaptured.
- `tests/QmlUiAuditTests.cpp` - `rhiViewportThumbnailCaptureUsesRealReadback` slot (stub gone, offscreen RT, readBackTexture, synchronize() queue, queued callback, single-sample, public contract preserved).

## Decisions Made
- Thumbnail pipelines are separate instances (`m_thumbnailFillPipeline`/`m_thumbnailLinePipeline`) bound to the thumbnail RPD, not reused from the on-screen pipelines, because the render-pass descriptors differ (the on-screen `m_renderPassDescriptor = renderTarget()->renderPassDescriptor()` is incompatible with the offscreen texture format).
- The offscreen pass reuses `m_srb` (binding 0 = camera uniform buffer) and re-uploads only the 64-byte MVP with the thumbnail aspect (1.0); the gizmoCenter/gizmoScale tail is left untouched (no gizmos drawn in the thumbnail).
- The readback is scheduled via `cb->resourceUpdate(readbackUpdates)` directly after the offscreen `endPass()` so it is part of the current frame's command stream (the alternative — merging into the next on-screen beginPass — would require an extra frame).
- The QImage is constructed as `QImage::Format_RGBA8888` from the raw readback bytes because `QRhiReadbackResult.format` is `QRhiTexture::Format` (RGBA8), and `Format_RGBA8888` matches that byte order exactly (no swizzle).

## Deviations from Plan

None - plan executed exactly as written. The three implementation tasks (95-01-01, 95-01-02, 95-01-03) and the audit test (95-01-04) landed in a single cohesive commit because the renderer cannot compile without the item-side fields and the test references both surfaces. The plan explicitly noted "all three tasks land in one commit wave" (splitting was for review clarity only).

## Issues Encountered
- The canonical build wrapper (`auto_verify_with_vcvars.ps1`) timed out (exit 1) during the test-target build phase (was building ViewModelSmokeTests MOC/UIC when the wrapper budget elapsed). This matches the documented prior-phase pattern: production code compiled and linked clean (`OWzxSlicer.exe` link OK at `[239/239]`). The regression ctest was then run directly via `run_unit_tests_vcvars.ps1` (no reconfigure), which built QmlUiAuditTests + the regression targets incrementally and ran them to completion. All targets PASSED.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- The real QRhi capture infrastructure is in place and source-audited. Phase 96 (3MF Thumbnail Write Integration) can populate `PlateData::plate_thumbnail` and `StoreParams::thumbnail_data` by triggering `requestThumbnailCapture` and consuming the `lastThumbnailData` / `thumbnailCaptured` contract that Phase 95 preserved.
- Runtime pixel proof (a non-solid-color capture under a loaded model) is routed to Phase 98 — Phase 95 proves the wiring is real via the source audit + regression ctest (no GPU/display path exercised here).

## Self-Check: PASSED

Plan-level verification commands re-run:
- `rg -n "#18222c" src/qml_gui/Renderer/RhiViewport.cpp` -> no matches (stub color removed) PASS
- `rg -n "readBackTexture|QRhiReadbackResult|newTextureRenderTarget|newCompatibleRenderPassDescriptor|m_thumbnailRequestPending = false|deliverThumbnail|Qt::QueuedConnection" ...` -> all anchors present PASS
- `git diff --check` -> exit 0 (no whitespace errors) PASS
- `encoding_guard.py` -> ok (5 files) PASS
- Canonical build: `OWzxSlicer.exe` link OK at [239/239] PASS (wrapper timed out in test-target phase; documented prior-phase pattern)
- Regression ctest via `run_unit_tests_vcvars.ps1`: PrepareSceneDataTests PASSED, ViewModelSmokeTests PASSED, QmlUiAuditTests PASSED (incl. new slot), PartPlateTests PASSED, PreviewParserTests PASSED PASS

---
*Phase: 95-qrhi-thumbnail-capture-infrastructure*
*Completed: 2026-07-10*
