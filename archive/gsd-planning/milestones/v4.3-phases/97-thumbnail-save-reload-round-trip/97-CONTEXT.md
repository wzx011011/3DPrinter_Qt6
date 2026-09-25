# Phase 97: Thumbnail Save-Reload Round-Trip - Context

**Gathered:** 2026-07-10
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 97 verifies the thumbnail save → reload round-trip and adds an automated
test. The read side is already complete (`ProjectServiceMock.cpp:5455-5466`
extracts `plate->plate_thumbnail` → QImage, applied at `5654-5660`); Phase 96
just closed the write side. Phase 97 closes the loop:

1. Verify that saving a project with a captured thumbnail, then reloading it,
   restores the thumbnail via the existing read side so the reloaded
   `PartPlate::thumbnail()` matches the saved pixels (THUMBRT-01).
2. Add an automated round-trip test asserting the thumbnail pixels survive
   save → reload (THUMBRT-02, THUMB-02 closure).

Phase 97 delivers:
- An automated test (PartPlateTests or a new round-trip test) that:
  - Creates a project with a known thumbnail (a test QImage with recognizable pixels, set via PartPlate::setThumbnail).
  - Saves it to a 3MF via saveProject (which now populates thumbnail_data/plate_thumbnail per Phase 96).
  - Reloads it via the load path (which extracts plate_thumbnail → QImage per the read side).
  - Asserts the reloaded PartPlate::thumbnail() is non-null and matches the saved pixels (within format tolerance — RGBA8888 round-trip should be exact).
- Any small wiring needed if the read side has a gap the round-trip exposes (but the read side was reported complete; Phase 97 mainly validates it end-to-end).

Out of scope for Phase 97:
- Mock generator removal + final verification + runtime capture evidence → Phase 98.
- The real QRhi capture itself (Phase 95) — Phase 97's test uses a known QImage directly, not requiring the RHI capture path.
- Auto filament-map, CLI fixtures, D3D12, GLGizmoMeasure — future.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion (discuss skipped — use ROADMAP goal + read/write side code)

1. **Test location:** Prefer PartPlateTests.cpp (it already has thumbnail tests
   from v3.2 THUMB-01: `thumbnailRoundTrip` in-memory). Add a new slot
   `thumbnailSaveReloadRoundTrip` that goes through the real saveProject → load
   path. If the save/load path isn't accessible from PartPlateTests (it needs
   ProjectServiceMock + HAS_LIBSLIC3R), put it in ViewModelSmokeTests.cpp or a
   new test that constructs a ProjectServiceMock.

2. **Test thumbnail source:** Use a small synthesized QImage (e.g., a known
   gradient or solid recognizable color) set via `PartPlate::setThumbnail`,
   NOT requiring the RHI capture. This isolates the round-trip test from the
   capture path (Phase 95) — it tests persistence only.

3. **Assertion:** After reload, assert `reloadedPlate.hasThumbnail()` is true
   and the reloaded thumbnail dimensions match + pixel bytes match (RGBA8888
   round-trip should be exact; allow a tolerance only if the PNG compression
   is lossless — it is, tdefl PNG is lossless). If exact byte match is fragile
   due to format conversion, assert non-null + same dimensions + recognizable
   pixel (e.g., corner pixel color).

4. **File path:** Use a temp file path (`QStandardPaths::TempLocation` or a
   `QTemporaryFile`) for the round-trip test so it doesn't pollute the repo.
   Guard the test with `#ifdef HAS_LIBSLIC3R` (the save/load needs libslic3r).

### Recommended approach (Claude's Discretion confirmed, noted for planning)
- Add `thumbnailSaveReloadRoundTrip` to PartPlateTests.cpp (or the most
  appropriate existing test file). The test: construct ProjectServiceMock, add
  a plate with a known QImage thumbnail, saveProject to a temp .3mf, create a
  fresh ProjectServiceMock, loadFile the .3mf, assert the plate thumbnail
  survived. Guard with HAS_LIBSLIC3R (QSKIP if unavailable, like existing tests).
- If saveProject/loadFile require additional setup (model, config), mirror the
  existing PartPlateTests save/reload fixtures.

</decisions>

<code_context>
## Existing Code Insights

### Round-trip endpoints (already implemented in prior phases)
- Write (Phase 96): `ProjectServiceMock.cpp:5138 pd->plate_thumbnail = qimageToThumbnailData(...)` + `:5189 params.thumbnail_data.push_back(&projectThumb)`.
- Writer (upstream): `bbs_3mf.cpp` encodes PNG via tdefl (lossless).
- Read (v3.2, complete): `ProjectServiceMock.cpp:5455-5466` extracts `plate->plate_thumbnail` → QImage (Format_RGBA8888), applied at `5654-5660` via `setThumbnail`.

### Existing thumbnail tests (the template)
- `tests/PartPlateTests.cpp` — has `thumbnailRoundTrip` (in-memory THUMB-01 test) and other thumbnail variant tests. The new `thumbnailSaveReloadRoundTrip` slots alongside these.
- PartPlateTests already constructs ProjectServiceMock + does save/load in some tests (check the file for existing save/reload fixtures to mirror).

### PartPlate API
- `src/core/model/PartPlate.h:122-124,255` — `thumbnail()`, `setThumbnail(QImage)`, `hasThumbnail()`.

### Test guards
- Existing PartPlateTests use `#ifdef HAS_LIBSLIC3R` + `QSKIP` when libslic3r is unavailable (the save/load needs it). Mirror this.

</code_context>

<specifics>
## Specific Ideas

- The round-trip test should use a KNOWN QImage (not RHI capture) so it isolates
  persistence from capture — Phase 95 (capture) and Phase 97 (persistence) are
  tested independently.
- PNG via tdefl is lossless, so an exact pixel match is achievable after the
  RGBA8888 round-trip. If the format conversion introduces a subtle difference,
  fall back to dimensions + recognizable-pixel assertion and document it.
- This test closes THUMB-02 (the v3.2 partial requirement): the round-trip is now
  verified, not just the in-memory cache.

</specifics>

<deferred>
## Deferred Ideas

- Mock generator removal + final verification + runtime capture evidence → Phase 98.
- Real RHI capture end-to-end runtime proof → Phase 98.

</deferred>

---

*Phase: 97-thumbnail-save-reload-round-trip*
*Context gathered: 2026-07-10 (discuss skipped via workflow.skip_discuss; read side complete, write side just closed by Phase 96)*
