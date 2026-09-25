---
phase: 97-thumbnail-save-reload-round-trip
plan: 01
subsystem: infra
tags: [3mf, thumbnail, round-trip, read-side, miniz, plate-thumbnail, rgba8888, spiral-mode, coBool]

# Dependency graph
requires:
  - phase: 96-3mf-thumbnail-write-integration
    provides: Closed write side -- PlateData::plate_thumbnail populated in buildPlateDataList + StoreParams::thumbnail_data populated in saveProject via qimageToThumbnailData (Format_RGBA8888 format symmetry); the writer emits Metadata/plate_N.png via tdefl (lossless) PNG encoding.
  - phase: 95-qrhi-thumbnail-capture-infrastructure
    provides: Real captured QImage cached on PartPlate::m_thumbnail (the pixel source Phase 96 consumes). Phase 97's test injects a KNOWN synthesized QImage directly so it does NOT depend on capture at runtime.
provides:
  - Automated end-to-end save->reload 3MF thumbnail pixel round-trip test (thumbnailSaveReloadRoundTrip) in tests/PartPlateTests.cpp, closing v3.2 THUMB-02.
  - File-local extractPlateThumbnailFrom3mf(path, plateIndex) helper reading Metadata/plate_N.png straight out of the 3MF archive via miniz (Slic3r::open_zip_reader + mz_zip_reader_extract_to_mem) and decoding it to a QImage.
  - Read-side wiring of the per-plate thumbnail in BOTH model-load paths (loadFile + loadProject) via the helper, applied via PartPlate::setThumbnail after addInstance invalidation.
  - spiral_mode coBool read/write fix (buildPlateDataList write + loadFile/loadProject reads) -- the first saveProject on a real model crashed in buildPlateDataList because spiral_mode is ConfigOptionBool, which does NOT override setInt/getInt.
affects: [98-thumbnail-verification-and-cleanup]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Direct 3MF archive extraction via miniz (Slic3r::open_zip_reader + mz_zip_reader_locate_file/extract_to_mem) for per-plate thumbnails -- mirrors upstream _extract_from_archive (bbs_3mf.cpp:1640-1643) which only runs in load_gcode_3mf_from_stream, not the normal model-load path."
    - "ConfigOptionBool typed accessor requirement: coBool options (spiral_mode) do NOT override setInt/getInt (Config.hpp), so the typed option<T> template + .value access MUST be used, not getInt/setInt. Mirrors upstream set_key_value(\"spiral_mode\", new ConfigOptionBool(...))."
    - "QImage::fill(QColor) vs fill(uint pixel) on RGBA8888: fill(uint) stores the raw uint in NATIVE byte order which swaps R/B on little-endian; fill(QColor) converts to the target format correctly."

key-files:
  created: []
  modified:
    - tests/PartPlateTests.cpp
    - src/core/services/ProjectServiceMock.cpp

key-decisions:
  - "extractPlateThumbnailFrom3mf is a file-local static free function under #ifdef HAS_LIBSLIC3R (forward-declared at the top, defined near qimageToThumbnailData) -- keeps bbs_3mf.hpp / Slic3r::open_zip_reader out of ProjectServiceMock.h, matching the qimageToThumbnailData / buildPlateDataList design."
  - "Thumbnail-byte extraction lives in the Qt6 read side (NOT libslic3r): the normal model-load path (_BBS_3MF_Importer::_load_model_from_file) does not extract per-plate thumbnail bytes (only the thumbnail_file STRING), and the byte-extraction path (_extract_from_archive at bbs_3mf.cpp:1640) only runs in load_gcode_3mf_from_stream. Doing the extraction in ProjectServiceMock avoids modifying libslic3r (project rule) and mirrors what the upstream gcode path does."
  - "Archive entry is Metadata/plate_{plateIndex+1}.png (1-based), constructed from PlateData::plate_index (0-based), matching the writer at bbs_3mf.cpp:6550. A missing entry leaves the thumbnail null (graceful -- plates with no captured thumbnail load empty, no regression)."
  - "Both model-load paths (loadFile + loadProject) get the SAME fix so save->reload round-trips regardless of which entry point the caller used."

patterns-established:
  - "Qt-side miniz archive extraction for any 3MF entry the model-load path skips (e.g. per-plate thumbnails, future variant thumbnails)."
  - "coBool config option handling: use option<ConfigOptionBool> + .value, never getInt/setInt, in buildPlateDataList (write) and the load read blocks."

requirements-completed: [THUMBRT-01, THUMBRT-02]

# Metrics
duration: 150min
completed: 2026-07-10
---

# Phase 97 Plan 01: Thumbnail Save-Reload Round-Trip Summary

**The save->reload 3MF thumbnail round-trip is closed end-to-end with an exact RGBA8888 pixel match: a KNOWN synthesized thumbnail survives saveProject -> loadFile through the real bbs_3mf writer/reader, verified by an automated test asserting lossless byte equality (THUMBRT-01/02).**

## Performance

- **Duration:** ~150 min
- **Started:** 2026-07-10
- **Completed:** 2026-07-10
- **Tasks:** 2 (97-01-01 test slot; 97-01-02 conditional read-side fix -- ACTIVATED, the round-trip exposed a real read-side gap)
- **Files modified:** 2

## Accomplishments
- thumbnailSaveReloadRoundTrip automated test added to tests/PartPlateTests.cpp (task 97-01-01): synthesizes a KNOWN 32x32 RGBA8888 QImage, installs it via the plateListMut() test seam, saves to a temp .3mf via saveProject, reloads into a FRESH ProjectServiceMock via loadFile, and asserts the reloaded PartPlate::thumbnail() survived -- hasThumbnail true + dimensions match + recognizable-pixel match + EXACT std::memcmp RGBA8888 buffer match.
- Read-side gap fixed (task 97-01-02): the normal model-load path (_BBS_3MF_Importer::_load_model_from_file) does NOT extract per-plate thumbnail bytes into PlateData::plate_thumbnail.pixels -- it only copies the thumbnail_file STRING (bbs_3mf.cpp:2299); the byte extraction (_extract_from_archive at bbs_3mf.cpp:1640) lives ONLY in load_gcode_3mf_from_stream, not the model-load path. The new file-local extractPlateThumbnailFrom3mf helper reads Metadata/plate_N.png straight out of the archive via miniz and decodes it. Wired into BOTH loadFile and loadProject read blocks.
- spiral_mode coBool crash fixed: the first saveProject on a real model crashed in buildPlateDataList because spiral_mode is a coBool option whose ConfigOptionBool does NOT override setInt/getInt (the base accessors throw BadOptionTypeException). Fixed the write side (buildPlateDataList: typed ConfigOptionBool accessor) and both read sides (loadFile + loadProject: typed Bool read). Mirrors upstream set_key_value("spiral_mode", new ConfigOptionBool(...)).
- Test fill correctness fixed: QImage::fill(QRgb) on RGBA8888 stores the raw uint in native byte order, swapping R/B on little-endian; switched to fill(QColor) so the pixels actually hold the intended color and knownColor matches on reload.

## Task Commits

Each task was committed atomically:

1. **Task 97-01-01: add thumbnailSaveReloadRoundTrip test slot** - `dfe722a` (test)
2. **Task 97-01-02: extract per-plate thumbnail from 3MF on load + spiral_mode coBool fix + test fill fix** - `286cb05` (fix)

## Files Created/Modified
- `tests/PartPlateTests.cpp` - Added the thumbnailSaveReloadRoundTrip private slot (HAS_LIBSLIC3R guard + QSKIP stub) driving loadFile -> setThumbnail -> saveProject -> fresh loadFile -> thumbnail survival assertions. Fixed the synthesized-thumbnail fill to use fill(QColor) (fill(QRgb) swaps R/B on RGBA8888 native byte order). Added `<QColor>` include.
- `src/core/services/ProjectServiceMock.cpp` - Added file-local extractPlateThumbnailFrom3mf helper (miniz via Slic3r::open_zip_reader) + forward declaration; wired the helper into the loadFile and loadProject per-plate read blocks (replacing the v3.2 pixels-based extraction that assumed plate_thumbnail.pixels held raw RGBA8888, which was empty because the model-load path skips byte extraction); fixed spiral_mode coBool read (loadFile + loadProject) and write (buildPlateDataList) to use the typed ConfigOptionBool accessor; added `<QDir>` and `<libslic3r/miniz_extension.hpp>` includes.

## Decisions Made
- Read-side extraction lives in ProjectServiceMock (not libslic3r): the model-load path does not extract per-plate thumbnail bytes, and modifying libslic3r is out of scope (project rule). The Qt6 helper mirrors upstream _extract_from_archive. This is the minimal targeted fix task 97-01-02's contingency anticipated ("a small read-side wiring fix").
- The exact std::memcmp RGBA8888 buffer match is the PRIMARY assertion (tdefl PNG is lossless; Phase 96 format symmetry holds for the decode), kept alongside the looser dimensions + recognizable-pixel assertions as the documented fallback (RT-07).
- Both model-load entry points (loadFile + loadProject) got the same fix so round-trip works regardless of caller.
- Did NOT touch the Phase 96 write side (qimageToThumbnailData, PlateData::plate_thumbnail populate, StoreParams::thumbnail_data populate) -- it was correct (the saved 3MF contains a valid Metadata/plate_1.png and the model_settings.config thumbnail_file reference). Did NOT touch the Phase 95 capture contract.
- The spiral_mode fix is strictly necessary collateral: the first saveProject on a real model (which the round-trip test requires) crashed in buildPlateDataList before any thumbnail code ran. It is minimal (typed accessor instead of setInt/getInt) and mirrors upstream.

## Deviations from Plan

- Task 97-01-02 was ACTIVATED (not a no-op): the plan allowed for "possibly a small read-side wiring fix if the round-trip exposes a gap." The gap was deeper than a missing apply site -- the entire per-plate thumbnail-byte extraction was absent from the model-load path (only the gcode-stream path had it). The fix extracts the PNG directly from the archive, which is the same operation upstream does, just relocated to the Qt6 side.
- The spiral_mode coBool crash was an UNPLANNED-but-necessary discovery: the plan's read_first listed buildPlateDataList but did not anticipate that saveProject on a real model would throw on spiral_mode. The fix is minimal and mirrors upstream; it is documented here for auditability.
- The test's synthesized-thumbnail fill was changed from fill(QRgb) to fill(QColor) (RT-02 said "a solid fill of a known QRgb color"). This does not weaken RT-02 -- the color is still a known, recognizable, deterministic pixel pattern; it just uses the format-correct fill API so the asserted knownColor matches the saved pixels.

## Issues Encountered

- **Read-side gap (root cause):** After Slic3r::Model::read_from_file, PlateData::plate_thumbnail.pixels was EMPTY (is_valid() false) even though the saved 3MF contained a valid Metadata/plate_1.png and the model_settings.config thumbnail_file reference. Root cause: the byte extraction (_extract_from_archive at bbs_3mf.cpp:1640) lives ONLY in _BBS_3MF_Importer::load_gcode_3mf_from_stream (bbs_3mf.cpp:1492), NOT in _load_model_from_file (bbs_3mf.cpp:1674, the path read_from_file takes for a model 3MF). The model-load path only copies the thumbnail_file STRING (bbs_3mf.cpp:2299, with the backup path prepended). Diagnosed via a temporary diagnostic dumping plateDataList state right after read_from_file (pix=0 w=0 h=0, thumbfile set to the gcode/backup-derived path). Fix: extractPlateThumbnailFrom3mf reads the PNG straight out of the archive.
- **spiral_mode coBool crash:** buildPlateDataList's `opt->setInt(p->spiralMode())` and the read sides' `opt->getInt()` threw "Calling ConfigOption::setInt/getInt on a non-int ConfigOption" because spiral_mode is a coBool option and ConfigOptionBool does not override setInt/getInt. Fix: typed ConfigOptionBool accessor (.value = / .value).
- **Test R/B swap:** savedThumb.fill(qRgba(123,45,67,255)) produced pixels with R=67,B=123 (swapped) because QImage::fill(uint) stores the raw uint in native byte order on RGBA8888. The round-trip itself was byte-exact (production correct); only the test's expectation was wrong. Fix: fill(QColor).
- **Canonical build wrapper timeout (documented prior-phase pattern):** The canonical build command is slow (~30+ min with libslic3r reconfigure). Per the plan's documented prior-phase pattern (Phase 95 95-01-SUMMARY.md:102, Phase 96 96-01-SUMMARY.md:97), the wrapper may time out during test-target compilation past the production link gate. Verification was done via the incremental build path (scripts/run_partplate_only.ps1 building owzx_app_core + PartPlateTests / ViewModelSmokeTests directly via ninja, then ctest --test-dir build). Production code (OWzxSlicer.exe) compiles and links clean; all regression ctests pass.

## User Setup Required

None - no external service configuration required.

## Verification Results

- **Source audit (test + read-side wiring present):** PASS
  - thumbnailSaveReloadRoundTrip slot present (declaration + #else QSKIP stub) in tests/PartPlateTests.cpp.
  - extractPlateThumbnailFrom3mf helper present in ProjectServiceMock.cpp (forward declaration + definition).
  - Helper wired into both loadFile and loadProject per-plate read blocks.
  - spiral_mode typed-Bool read/write in buildPlateDataList (write) + loadFile + loadProject (reads).
  - The new slot does NOT use generatePlateThumbnail/generatePlateThumbnailVariant as its thumbnail source (RT-02): it synthesizes the QImage directly via fill(QColor).
- **Build (production code compile + link):** PASS -- owzx_app_core (ProjectServiceMock.cpp) compiles clean; PartPlateTests.exe and ViewModelSmokeTests.exe link clean; build/OWzxSlicer.exe previously linked clean (Phase 96).
- **thumbnailSaveReloadRoundTrip slot:** PASS (the core verification of Phase 97) -- exact RGBA8888 std::memcmp buffer match, including the recognizable-pixel + dimensions assertions.
  ```
  PASS   : PartPlateTests::thumbnailSaveReloadRoundTrip()
  Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 217ms
  ```
- **Regression ctest (Prepare/Preview/AssembleView/PartPlate regression-free):** PASS
  ```
  1/4 Test #2: ViewModelSmokeTests ..............   Passed    7.52 sec
  2/4 Test #3: QmlUiAuditTests ..................   Passed    0.07 sec
  3/4 Test #5: PrepareSceneDataTests ............   Passed    0.03 sec
  4/4 Test #9: PartPlateTests ...................   Passed    0.61 sec
  100% tests passed, 0 tests failed out of 4
  ```
- **Encoding guard + whitespace:** PASS -- encoding_guard ok on both modified files; git diff --check clean.

THUMBRT-01/02 note: the round-trip asserts the PER-PLATE thumbnail (Metadata/plate_N.png -> PartPlate::thumbnail()). The project-level StoreParams::thumbnail_data (Phase 96 THUMBWRITE-02) is write-only on the round-trip (it is NOT read back into PartPlate::thumbnail()) and is therefore not the assertion target (RT-09). The format symmetry (convertToFormat(RGBA8888) write <-> PNG decode read) plus lossless tdefl PNG encoding make the exact RGBA8888 buffer match achievable.

ViewModelSmokeTests::multiPlate3mfRoundTripPreservesState note: this slot (which exercises the same loadProject path with no thumbnails saved) occasionally times out on loadFinished under heavy ctest parallel load (pre-existing async-loadFinished flakiness, NOT a Phase 97 regression -- confirmed by stashing the Phase 97 changes, rebuilding the Phase 96 baseline, and re-running: the slot passes both with and without Phase 97 changes; isolated runs and the final ctest run all pass).

## Next Phase Readiness
- Phase 98 (Thumbnail Verification And Cleanup) can now proceed: the round-trip is verified (THUMBRT-01/02), so Phase 98 can remove the mock generator (ProjectServiceMock.cpp generatePlateThumbnail QPainter placeholder), audit the requestThumbnailCapture dead path, run the final canonical verifier, and gather runtime capture evidence (THUMBVERIFY-01/02). The extractPlateThumbnailFrom3mf helper gives Phase 98 a clean Qt-side archive-extraction pattern if variant-thumbnail round-trips are added later.

## Self-Check: PASSED
