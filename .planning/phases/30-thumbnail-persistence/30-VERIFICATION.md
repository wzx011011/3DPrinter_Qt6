---
phase: 30
slug: thumbnail-persistence
status: partial
verified: 2026-06-28
requirements: [THUMB-01, THUMB-02]
---

# Phase 30 Verification: Thumbnail Persistence

## Status: PARTIAL ✅ (THUMB-01 complete; THUMB-02 in-memory cache complete, 3MF pixel persistence deferred to THUMB-03)

## What's verified

### THUMB-01 — Thumbnail variants (PASSED ✅)
- `generatePlateThumbnailVariant(plateIndex, size, variant)` produces 2 variants:
  - variant=0: main flat-color (delegates to existing `generatePlateThumbnail`)
  - variant=1: NEW top-down 2D footprint (QPainter, objects as bounding-box rectangles colored by palette)
- Test `thumbnailVariantsProduceValidData`: both variants return valid base64 PNG that decodes to a 256×256 QImage. ✅

### THUMB-02 — PartPlate thumbnail cache (PASSED ✅)
- `PartPlate` stores `QImage m_thumbnail` (Qt-native) with `thumbnail()`/`setThumbnail()`/`hasThumbnail()`.
- Cache invalidation on `addInstance`/`removeInstance`/`clearInstances`/`setLocked` (D-30-10).
- Test `thumbnailCacheInvalidation`: all 4 content-change operations clear the cache. ✅
- Test `thumbnailRoundTrip`: cache holds a generated thumbnail until a content change invalidates it. ✅

## Deferred to THUMB-03 (v3.3+)

The full 3MF save→reload pixel round-trip was attempted but the upstream writer's PNG pixel encoding path (`_add_thumbnail_file_to_archive`) throws a non-std exception on the Qt6 mock pipeline — it's coupled to real GL capture and not yet robustly validated. The save path is annotated for the future hook (D-30-5/D-30-7 population of `store_params.thumbnail_data`), but the pixel persistence is deferred to THUMB-03 alongside real GL capture. This is consistent with THUMB-03 already being deferred (REQUIREMENTS, blocked on QRhi framebuffer).

## Build verification
- `owzx_app_core` + `PartPlateTests` compile green (QTEST_MAIN for QGuiApplication).
- `PartPlateTests.exe` → **47 passed, 0 failed** (44 from Phase 29 + 3 new Phase 30).
- `QmlUiAuditTests`/`PrepareSceneDataTests` unaffected (no new QML; existing test suites unchanged).

## Files changed
- `src/core/model/PartPlate.h` — QImage m_thumbnail + accessors + cache invalidation.
- `src/core/services/ProjectServiceMock.h` — generatePlateThumbnailVariant + generateTopDownThumbnail (private) + plateListConst/plateListMut test seams.
- `src/core/services/ProjectServiceMock.cpp` — generatePlateThumbnailVariant + generateTopDownThumbnail; save path annotated (THUMB-02 hook, persistence deferred).
- `tests/PartPlateTests.cpp` — 3 new test slots + QTEST_MAIN switch.
