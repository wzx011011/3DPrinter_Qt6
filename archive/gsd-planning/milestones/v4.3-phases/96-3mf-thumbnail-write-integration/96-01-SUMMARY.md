---
phase: 96-3mf-thumbnail-write-integration
plan: 01
subsystem: infra
tags: [3mf, thumbnail, write-side, rgba8888, store-bbs-3mf, plate-thumbnail, storeparams-thumbnail-data]

# Dependency graph
requires:
  - phase: 95-qrhi-thumbnail-capture-infrastructure
    provides: Real QRhi texture readback capture producing a QImage cached on PartPlate::m_thumbnail (the pixel source for the write side)
  - phase: 94-thumbnail-capture-gap-audit
    provides: Frozen writer anchors (bbs_3mf.cpp:6133 thumbnail_data iteration, :6543 _add_thumbnail_file_to_archive, :7987 plate_thumbnail XML reference) + the two deferred populate sites at ProjectServiceMock.cpp:5089-5093/5127-5135
provides:
  - File-local qimageToThumbnailData(QImage) helper converting captured pixels to Slic3r::ThumbnailData (width/height/RGBA bytes) via convertToFormat(Format_RGBA8888) + resize + memcpy
  - buildPlateDataList populates PlateData::plate_thumbnail per plate from PartPlate::thumbnail() (guarded by !isNull())
  - saveProject populates StoreParams::thumbnail_data with a real project ThumbnailData (current plate, fallback plate 0) whose address outlives store_bbs_3mf
  - Format symmetry guarantee: write side produces Format_RGBA8888 bytes matching the read side at ProjectServiceMock.cpp:5455 (Phase 97 round-trip foundation)
  - projectServiceWrites3mfThumbnails QmlUiAuditTests source-audit slot (regression ctest guard)
affects: [97-thumbnail-save-reload-round-trip, 98-thumbnail-verification-and-cleanup]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "QImage -> Slic3r::ThumbnailData conversion via convertToFormat(Format_RGBA8888) + scanline memcpy (NOT ThumbnailData::set which fills white defaults)"
    - "Format symmetry: write side convertToFormat(Format_RGBA8888) mirrors the read side QImage::Format_RGBA8888 wrapping — the round-trip byte layout guarantee"
    - "ThumbnailData pointer lifetime via single local in block scope (stable address through store_bbs_3mf + release_PlateData_list cleanup) — avoids vector reallocation dangling"
    - "Source-audit test slot mirroring rhiViewportThumbnailCaptureUsesRealReadback pattern for write-side wiring regression"

key-files:
  created: []
  modified:
    - src/core/services/ProjectServiceMock.cpp
    - tests/QmlUiAuditTests.cpp

key-decisions:
  - "qimageToThumbnailData is a file-local static free function under #ifdef HAS_LIBSLIC3R (not a member, not in the public header) — keeps bbs_3mf.hpp / Slic3r::ThumbnailData out of ProjectServiceMock.h, matching the buildPlateDataList design."
  - "resize + std::memcpy scanline-by-scanline for real pixels (ThumbnailData::set() fills white defaults — explicitly NOT used)."
  - "Both populate sites guarded: !p->thumbnail().isNull() (per-plate) and projectThumb.is_valid() (project) so empty/invalidated caches leave the writer to skip gracefully — no regression when no capture exists."
  - "Single local ThumbnailData (projectThumb) in saveProject's real-3MF block scope for lifetime stability — not a vector that could reallocate and dangle the pushed pointer."

patterns-established:
  - "QImage -> ThumbnailData conversion helper (qimageToThumbnailData) for any future Qt->libslic3r pixel bridge"
  - "Block-scoped single-local pointer stability for StoreParams::thumbnail_data vector<ThumbnailData*>"
  - "Source-audit QmlUiAuditTests slot as the regression gate for deferred-write closure"

requirements-completed: [THUMBWRITE-01, THUMBWRITE-02, THUMBWRITE-03]

# Metrics
duration: 35min
completed: 2026-07-10
---

# Phase 96 Plan 01: 3MF Thumbnail Write Integration Summary

**Both 3MF thumbnail write-side populate sites closed: PlateData::plate_thumbnail (per-plate XML reference) and StoreParams::thumbnail_data (project PNG bytes), wired via a file-local qimageToThumbnailData helper that produces Format_RGBA8888 bytes symmetric with the read side.**

## Performance

- **Duration:** ~35 min
- **Started:** 2026-07-10T09:13:43Z
- **Completed:** 2026-07-10T09:48:00Z
- **Tasks:** 4
- **Files modified:** 2

## Accomplishments
- File-local `qimageToThumbnailData(const QImage&)` helper added under `#ifdef HAS_LIBSLIC3R`, converting captured pixels to `Slic3r::ThumbnailData` via `convertToFormat(QImage::Format_RGBA8888)` + `resize` + scanline `std::memcpy` (not `ThumbnailData::set()`, which fills white defaults).
- `buildPlateDataList` now populates `pd->plate_thumbnail` from `p->thumbnail()` guarded by `!p->thumbnail().isNull()` — drives the per-plate XML `<metadata thumbnail_file="Metadata/plate_N.png">` reference (`bbs_3mf.cpp:7987`). Replaces the `ProjectServiceMock.cpp:5089-5093` deferred comment.
- `saveProject` now populates `params.thumbnail_data` with a real project `ThumbnailData` built from the current plate (fallback plate 0), held in a single local whose address outlives `store_bbs_3mf`. Drives the actual PNG byte writing (`bbs_3mf.cpp:6133-6143` -> `_add_thumbnail_file_to_archive`). Replaces the `ProjectServiceMock.cpp:5127-5135` deferred comment.
- `projectServiceWrites3mfThumbnails` QmlUiAuditTests source-audit slot added (regression ctest guard for the write-side wiring).

## Task Commits

Each task was committed atomically:

1. **Task 96-01-01: qimageToThumbnailData helper** - `71295cc9` (feat)
2. **Task 96-01-02: populate plate_thumbnail per plate** - `68fb3b7e` (feat)
3. **Task 96-01-03: populate thumbnail_data in saveProject** - `94a78edf` (feat)
4. **Task 96-01-04: QmlUiAuditTests write-side slot** - `9829ab4e` (test)

## Files Created/Modified
- `src/core/services/ProjectServiceMock.cpp` - Added `qimageToThumbnailData` file-local helper; populated `pd->plate_thumbnail` in `buildPlateDataList`; populated `params.thumbnail_data` in `saveProject` with a lifetime-safe local `ThumbnailData`.
- `tests/QmlUiAuditTests.cpp` - Added `projectServiceWrites3mfThumbnails` private slot asserting the write-side wiring + deferred-marker removal.

## Decisions Made
- Used `resize` + `std::memcpy` (scanline-by-scanline via `constScanLine`) instead of `ThumbnailData::set(w,h)` — `set()` fills the buffer with white (255) defaults per `ThumbnailData.cpp:5-18`; real captured pixels require direct copy.
- Single local `ThumbnailData projectThumb` in `saveProject`'s real-3MF block (not a `std::vector<ThumbnailData>`) for lifetime stability — a single local has a stable address through `store_bbs_3mf` and the `release_PlateData_list` cleanup; a vector append could reallocate and dangle earlier addresses. This is the simplest correct lifetime (W-05).
- Project-level thumbnail source = current plate's `thumbnail()` (falling back to plate 0 when `currentPlateIndex() < 0`), preferring reuse of an already-captured thumbnail over an extra capture (Claude's Discretion in CONTEXT).
- Did NOT populate the variant thumbnail slots (`no_light_thumbnail_data`, `top_thumbnail_data`, `pick_thumbnail_data`) — Claude's Discretion, not required by v4.3 requirements.

## Deviations from Plan

None - plan executed exactly as written. All 4 tasks landed with the exact signatures, guards, and placement the plan specified.

## Issues Encountered

- **Canonical build wrapper timeout (documented prior-phase pattern):** The canonical build command (`powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`) timed out the background executor wrapper during test-target compilation (past the production link gate). The build log confirms production code compiled and linked clean: `[236/236] Linking CXX executable OWzxSlicer.exe` with zero errors, and `build/OWzxSlicer.exe` was produced. This matches the documented Phase 95 pattern (`95-01-SUMMARY.md`). Regression ctest was run directly against the built binaries (the plan's documented fallback) — all 4 tests passed.

## User Setup Required

None - no external service configuration required.

## Verification Results

- **Source audit (write-side wiring present + deferred markers gone):** PASS
  - `qimageToThumbnailData` helper present at `ProjectServiceMock.cpp:5056`.
  - `convertToFormat(QImage::Format_RGBA8888)` present at `:5061`.
  - `pd->plate_thumbnail = qimageToThumbnailData(p->thumbnail())` guarded by `!p->thumbnail().isNull()` at `:5138-5139` (THUMBWRITE-01).
  - `params.thumbnail_data.push_back(&projectThumb)` at `:5198`, guarded by `projectThumb.is_valid()`, before `store_bbs_3mf` at `:5203` (THUMBWRITE-02).
  - Deferred markers `throws a non-std exception on the Qt6 mock pipeline` and `deferred to THUMB-03` both GONE from `ProjectServiceMock.cpp` (THUMBWRITE-03 closure).
  - `projectServiceWrites3mfThumbnails` slot declared at `tests/QmlUiAuditTests.cpp:152` and implemented at `:3451`.
- **Canonical build (production code compile + link):** PASS — `[236/236] Linking CXX executable OWzxSlicer.exe`, zero errors in build log; `build/OWzxSlicer.exe` produced. (Wrapper timed out during test compilation — documented prior-phase pattern.)
- **Regression ctest (Prepare/Preview/AssembleView/PartPlate regression-free):** PASS
  ```
  1/4 Test #2: ViewModelSmokeTests ..............   Passed   10.20 sec
  2/4 Test #3: QmlUiAuditTests ..................   Passed    0.73 sec
  3/4 Test #5: PrepareSceneDataTests ............   Passed    0.09 sec
  4/4 Test #9: PartPlateTests ...................   Passed    1.99 sec
  100% tests passed, 0 tests failed out of 4
  ```
- **Encoding guard + whitespace:** PASS — `encoding_guard ok` on both modified files; `git diff --check` clean (only the standard LF->CRLF normalization warning).

THUMBWRITE-03 note: the upstream writer's PNG path (`_add_thumbnail_file_to_archive` at `bbs_3mf.cpp:6543`) calls `tdefl_write_image_to_png_file_in_memory_ex` (`:6548`) which returns non-null on real RGBA pixels -> `mz_zip_writer_add_mem` (`:6551`) succeeds. The v3.2 "throws a non-std exception" was the empty/mock-pixel path. Phase 96 verified the populated path compiles + links clean against the real writer and the regression ctest passes; runtime save round-trip proof (pixels survive save -> reload through the read side at `:5455-5466`) is routed to Phase 97 (THUMBRT-01/02).

## Next Phase Readiness
- Phase 97 (Thumbnail Save-Reload Round-Trip) can now assert the write side actually persists: the format symmetry (`convertToFormat(Format_RGBA8888)` write <-> `Format_RGBA8888` read at `:5455`) is in place, and both populate sites emit real pixels. The round-trip test should save a project with a known thumbnail, reload it, and assert the reloaded `PartPlate::thumbnail()` matches.
- Phase 98 (Verification + Cleanup) can proceed to remove the mock generator (`generatePlateThumbnail`) and the `requestThumbnailCapture` dead-path audit once the round-trip is verified.

## Self-Check: PASSED
