# Phase 96: 3MF Thumbnail Write Integration - Context

**Gathered:** 2026-07-10
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 96 closes the 3MF thumbnail write side. Phase 95 now delivers real
captured pixels (via `lastThumbnailData`/`thumbnailCaptured`); Phase 96 converts
those pixels into `ThumbnailData` (width/height/RGBA) and populates the two
write-side hooks that were explicitly omitted in v3.2:

1. `buildPlateDataList` populates `PlateData::plate_thumbnail` per plate (the
   `ProjectServiceMock.cpp:5089-5093` deferred write).
2. `saveProject` populates `StoreParams::thumbnail_data` (the
   `ProjectServiceMock.cpp:5127-5135` deferred write, read by the writer at
   `bbs_3mf.cpp:6133`).

The upstream writer's PNG encoding path (`_add_thumbnail_file_to_archive`,
`bbs_3mf.cpp:5879`) uses `tdefl_write_image_to_png_file_in_memory_ex` over
`ThumbnailData::pixels`. With real RGBA pixels populated, this encodes cleanly —
the prior "throws a non-std exception" was the empty/mock-pixel path; with real
pixels it works (returns non-null png_data → mz_zip_writer_add_mem succeeds).

Phase 96 delivers:
- A QImage → `ThumbnailData` (width/height/RGBA byte vector) converter.
- `PlateData::plate_thumbnail` populated from the captured thumbnail per plate.
- `StoreParams::thumbnail_data` populated with a real project thumbnail.
- `store_bbs_3mf` runs to completion with thumbnails (no exception, no silent skip).

Out of scope for Phase 96:
- Save-reload round-trip test → Phase 97.
- Mock generator removal + final verification → Phase 98.
- The other thumbnail variant slots (`no_light_thumbnail_data`, `top_thumbnail_data`,
  `pick_thumbnail_data` at `bbs_3mf.hpp:236-238`) — populate the primary
  `thumbnail_data` + `plate_thumbnail` first; variants are Claude's Discretion but
  not required by the v4.3 requirements.

</domain>

<decisions>
## Implementation Decisions

### Phase 94 Frozen Decisions (inherited) + Phase 95 output
- Phase 95 preserves the `lastThumbnailData` (base64 PNG) + `thumbnailCaptured`
  signal contract (consumed by PreparePage.qml + now Phase 96).
- The capture produces a real `QImage` (delivered to the GUI thread via the
  queued callback). Phase 96 converts that QImage into `ThumbnailData`.

### Locked decisions
1. **QImage → ThumbnailData conversion:** extract width/height, convert to RGBA8888
   (`QImage::Format_RGBA8888` via `convertToFormat` if needed), copy the pixel bytes
   into `ThumbnailData::pixels` (a `std::vector<unsigned char>` of width*height*4).
   Set `ThumbnailData::width`/`height`. This matches the read-side format
   (`ProjectServiceMock.cpp:5455` uses `Format_RGBA8888` wrapping of
   `ThumbnailData::pixels`).

2. **PlateData::plate_thumbnail population:** in `buildPlateDataList`, after the
   existing per-plate loop body, set `pd->plate_thumbnail` from the plate's cached
   `PartPlate::thumbnail()` (which Phase 95 capture + Phase 97 round-trip will keep
   populated). If the cache is empty (no capture yet), leave `plate_thumbnail` empty
   (writer silently skips — graceful).

3. **StoreParams::thumbnail_data population:** in `saveProject`, before
   `store_bbs_3mf(params)`, build a `ThumbnailData` from a captured project thumbnail
   (e.g., the current plate's thumbnail, or a dedicated project-level capture) and
   push its pointer into `params.thumbnail_data`. Manage the ThumbnailData lifetime
   to outlive the `store_bbs_3mf` call (local variable or container that lives until
   after the call).

4. **PNG encoding path no longer throws:** with real RGBA pixels in
   `ThumbnailData::pixels`, `tdefl_write_image_to_png_file_in_memory_ex` returns
   non-null → `mz_zip_writer_add_mem` succeeds → no exception, no silent skip. The
   v3.2 "throws" concern was the empty-pixel path; verify in Phase 96 that the
   populated path completes, and document if any edge case remains.

### Claude's Discretion
- Whether to populate the variant thumbnail slots (`no_light`/`top`/`pick`) — not
  required by v4.3 requirements; skip unless trivial.
- The exact project-level thumbnail source (current plate's thumbnail vs a separate
  capture). Prefer reusing an already-captured thumbnail to avoid an extra capture.
- Lifetime management details for the ThumbnailData (local var, member, unique_ptr
  container) as long as it outlives store_bbs_3mf.

### Recommended approach (Claude's Discretion confirmed, noted for planning)
- Add a `qimageToThumbnailData(const QImage&)` helper (file-local or in a small util)
  returning a `ThumbnailData` — mirrors the read-side `Format_RGBA8888` wrapping so
  the round-trip format is symmetric.
- In `buildPlateDataList`: `pd->plate_thumbnail = qimageToThumbnailData(p->thumbnail())`
  guarded by `!p->thumbnail().isNull()`.
- In `saveProject`: build one project `ThumbnailData` from the current plate's
  thumbnail (or PartPlateList plate 0), push its address into `params.thumbnail_data`,
  keep the object alive in a local until after `store_bbs_3mf`.
- Wrap the new population in `#ifdef HAS_LIBSLIC3R` (same guard as the existing
  store call).

</decisions>

<code_context>
## Existing Code Insights

### Write-side hooks (the deferred gaps to fill)
- `src/core/services/ProjectServiceMock.cpp:5089-5093` — `buildPlateDataList` comment: plate_thumbnail deferred. Fill after the existing filament_maps block.
- `src/core/services/ProjectServiceMock.cpp:5127-5135` — `saveProject` comment: thumbnail_data deferred. Fill before `store_bbs_3mf(params)` at `:5140`.

### Upstream writer (the consumer — confirms real pixels work)
- `third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.cpp:5879` — `_add_thumbnail_file_to_archive` impl: `tdefl_write_image_to_png_file_in_memory_ex(pixels.data(), width, height, 4, ...)` → non-null on real pixels → `mz_zip_writer_add_mem`.
- `bbs_3mf.cpp:6133,6137` — writer iterates `store_params.thumbnail_data`, calls `_add_thumbnail_file_to_archive(archive, *thumbnail_data[index], "Metadata/plate", index, true)`.
- `bbs_3mf.hpp:80` — `ThumbnailData plate_thumbnail;` (per-plate field).
- `bbs_3mf.hpp:227-239` — `StoreParams` with `thumbnail_data` (vector of ThumbnailData*).

### Read side (already complete — Phase 97 verifies round-trip)
- `ProjectServiceMock.cpp:5455-5466` — extracts `plate->plate_thumbnail` → QImage via `Format_RGBA8888`. This confirms the symmetric format Phase 96 must produce.

### ThumbnailData type
- `third_party/OrcaSlicer/src/libslic3r/Format/3mf.hpp` / `bbs_3mf.hpp` / `GCode/ThumbnailData.hpp` — `width`, `height`, `pixels` (std::vector<unsigned char>, RGBA).

### Phase 95 output (the pixel source)
- `RhiViewport::lastThumbnailData` (base64 PNG) + `thumbnailCaptured` signal — the captured thumbnail. Phase 96 may decode this back to QImage, OR (preferred) arrange for EditorViewModel/ProjectServiceMock to receive the QImage directly from the capture callback and cache it on PartPlate.

### PartPlate cache (the per-plate source)
- `src/core/model/PartPlate.h:122-124,255` — `thumbnail()`/`setThumbnail(QImage)`.

</code_context>

<specifics>
## Specific Ideas

- The round-trip format symmetry matters: read side uses `Format_RGBA8888`, so the
  write side must produce RGBA8888 bytes. `qimageToThumbnailData` must
  `convertToFormat(Format_RGBA8888)` before copying pixels.
- The writer takes `vector<ThumbnailData*>` — pointers must remain valid for the
  duration of `store_bbs_3mf`. A local `ThumbnailData` (or a small container on the
  stack) whose address is pushed works; ensure it isn't destroyed before the call.
- The populate code goes under `#ifdef HAS_LIBSLIC3R` (the existing store call is
  guarded at `:5109`).

</specifics>

<deferred>
## Deferred Ideas

- Save-reload round-trip test → Phase 97.
- Mock generator removal + final verification + runtime capture evidence → Phase 98.
- Thumbnail variant slots (no_light/top/pick) — Claude's Discretion, not required.

</deferred>

---

*Phase: 96-3mf-thumbnail-write-integration*
*Context gathered: 2026-07-10 (discuss skipped via workflow.skip_discuss; upstream writer PNG path confirmed to work on real pixels)*
