# Phase 30: Thumbnail Persistence - Context

**Gathered:** 2026-06-28
**Status:** Ready for planning
**Mode:** Smart Discuss (autonomous) — all 3 grey areas accepted verbatim

<domain>
## Phase Boundary

Implement thumbnail persistence for multi-plate projects: store per-plate thumbnails in `PartPlate` (Qt-native `QImage`), expose them via QML-friendly data URLs, generate 2 thumbnail variants (main flat-color + new top-down 2D footprint), and persist the main variant through 3MF save/load via `PlateData::plate_thumbnail`.

This phase closes REQUIREMENTS THUMB-01 (2 thumbnail variants) and THUMB-02 (3MF persistence round-trip). It builds on Phase 16's PartPlate domain + Phase 29's plate-grid. Real GL-capture thumbnails (THUMB-03, 4 variants incl. no-light/pick) are deferred to v3.3+ (blocked on QRhi framebuffer).

**In scope:**
- THUMB-01: Two thumbnail variants — (a) main flat-color 3D-ish look (current `generatePlateThumbnail`), (b) NEW top-down 2D footprint (QPainter, objects as bounding-box rectangles colored by palette, true top-down). New `Q_INVOKABLE QString generatePlateThumbnailVariant(int plateIndex, int size, int variant)`.
- THUMB-02: `PartPlate` stores `QImage m_thumbnail` (cached, invalidated on instance/lock change). `buildPlateDataList()` populates `PlateData::plate_thumbnail` (Slic3r::ThumbnailData) via QImage→RGBA bridge. Save/load round-trip test.
- Cache invalidation on content change (addInstance/removeInstance/clearInstances/setPlateLocked).
- Q_INVOKABLE `plateThumbnail(plateIndex, size, variant)` returning base64 PNG data URL.

**Out of scope:**
- Real GL-capture thumbnails (THUMB-03, v3.3+ — blocked on QRhi framebuffer; needs actual scene rendering).
- 4-variant upstream set (no_light, pick) — deferred with THUMB-03.
- Persisting the top variant to 3MF (`top_file` mechanism) — needs real GL capture (THUMB-03); top variant is in-memory UI only in Phase 30.
- Auto-generation tied to GL scene dirty events — Phase 30 auto-regenerates on plate-content change (simpler, no GL coupling).
- Thumbnail rendering in the plate sidebar QML — that's an existing UI hook (`generatePlateThumbnail` already wired); Phase 30 adds the variant + persistence, the QML binding benefits transparently.

</domain>

<decisions>
## Implementation Decisions

All three grey areas were accepted verbatim. The "complete source-truth alignment" pattern held for the variant semantics and persistence mechanism; the QImage-on-PartPlate decision is a pragmatic Qt-native choice (vs upstream's libslic3r ThumbnailData coupling).

### Thumbnail Variants (Area 1 — accepted verbatim)
- **D-30-1:** Generate **2 variants** (THUMB-01 minimum): (a) main flat-color 3D-ish look (current `generatePlateThumbnail`), (b) NEW top-down 2D footprint. Mirrors upstream `thumbnail_data` + `top_thumbnail_data` semantic split. Full 4-variant set (no_light, pick) deferred to THUMB-03 (v3.3+, blocked on QRhi).
- **D-30-2:** Top-down 2D footprint via **QPainter** — render the plate viewed from above (no 3D tilt), each object drawn as its bounding-box rectangle (scaled by dimensions), colored by palette. Mirrors upstream `top_thumbnail` camera angle. Do NOT wait for QRhi framebuffer (blocked — THUMB-03).
- **D-30-3:** New `Q_INVOKABLE QString generatePlateThumbnailVariant(int plateIndex, int size, int variant)` where `variant=0` (main), `variant=1` (top). Existing `generatePlateThumbnail` stays as `variant=0` (backward-compat). QML can request either.
- **D-30-4:** Persist the **main** variant to `PlateData::plate_thumbnail` (upstream's primary thumbnail). The top variant is generated in-memory for UI but not persisted (upstream `top_file` path needs real GL capture — THUMB-03 deferred).

### 3MF Persistence (Area 2 — accepted verbatim)
- **D-30-5:** Populate `PlateData::plate_thumbnail` in `buildPlateDataList()` (ProjectServiceMock.cpp:4806+) — set from the plate's stored thumbnail via a new `PartPlate::thumbnail()` getter. If the plate has no cached thumbnail, generate it (via `generatePlateThumbnail` mock for the main variant).
- **D-30-6:** Thumbnails are **auto-regenerated on plate content change** (after createPlate/addObject/duplicate/delete/move/lock) so the cache stays fresh and 3MF save writes current data. Mirrors upstream where thumbnails re-render on edit.
- **D-30-7:** ThumbnailData pixels set via `QImage` bridge: `generatePlateThumbnail` returns base64 PNG → `QImage::fromData(base64Decoded)` → convert Format_RGBA8888 → `ThumbnailData::set(w,h)` + `pixels = RGBA bytes`. Mirrors upstream `ThumbnailData::set` + pixel fill.
- **D-30-8:** **THUMB-02 has a deterministic save/reload round-trip test:** build a plate, add objects, save → reload → assert the reloaded plate's thumbnail is non-empty and matches the expected size. This is the load-bearing test for THUMB-02.

### PartPlate Domain + Cache (Area 3 — accepted verbatim)
- **D-30-9:** `PartPlate` stores `QImage m_thumbnail` (Qt-native, QML-friendly). `buildPlateDataList` converts to `Slic3r::ThumbnailData` bytes at save time. Do NOT store `Slic3r::ThumbnailData` directly (couples libslic3r into the PartPlate domain — keep the domain pure).
- **D-30-10:** Cache invalidation on **every content change** — addInstance/removeInstance/clearInstances/setPlateLocked set `m_thumbnail = QImage()` so the next access regenerates. Mirrors upstream cache-invalidation pattern.
- **D-30-11:** Expose via `Q_INVOKABLE QString plateThumbnail(int plateIndex, int size, int variant)` returning a base64 PNG data URL (matches `generatePlateThumbnail`'s existing shape). QML binds `<Image source: data URL>` directly.
- **D-30-12:** Test scope = **unit tests on PartPlate** (cache invalidation: thumbnail empty after content change) **+ integration tests on ProjectServiceMock** (save/reload round-trip, THUMB-02) **+ integration tests for generatePlateThumbnailVariant** (both variants produce valid non-empty base64 PNG). Mirrors the Phase 16/29 test-first pattern.

### Claude's Discretion
- Exact thumbnail dimensions for the 2 variants (recommend 256×256 for main, 512×512 for top — match upstream THUMBNAIL_SIZE spirit; size param overrides).
- The palette colors for the top-down footprint object rectangles (recommend reusing the existing color logic from `generatePlateThumbnail`'s flat-color rendering).
- Whether `QImage` on PartPlate needs `#include <QImage>` guarded or always-available (Qt6 core, always available — no guard).
- Whether the top-down footprint draws object names/labels (recommend NO for Phase 30 — keep it minimal; labels are a refinement).
- How `buildPlateDataList` decides which variant to persist (recommend: always main/variant=0; document that top variant is UI-only).
- Exact save-path hook: whether to regenerate in `buildPlateDataList` or in the save lambda itself (recommend `buildPlateDataList` — single source of truth for PlateData construction).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`ProjectServiceMock::generatePlateThumbnail(int, int)`** (ProjectServiceMock.cpp:4119+) — current main variant generator (flat-color 3D-ish look). Returns base64 PNG data URL. D-30-3 keeps this as variant=0; D-30-1 adds variant=1.
- **`ProjectServiceMock::buildPlateDataList()`** (ProjectServiceMock.cpp:4806+) — constructs `Slic3r::PlateData` per plate for 3MF save. Currently does NOT populate `plate_thumbnail`. D-30-5 adds that.
- **`Slic3r::PlateData::plate_thumbnail`** (`bbs_3mf.hpp:80`) — `ThumbnailData` field; `is_valid()` gates whether the 3MF writer emits the `THUMBNAIL_FILE_ATTR` metadata + writes the pixels (bbs_3mf.cpp:7987). D-30-5/D-30-7 populate this.
- **`Slic3r::ThumbnailData`** (`ThumbnailData.hpp`) — `{width, height, std::vector<unsigned char> pixels}`, `set(w,h)`, `is_valid()`, `reset()`. D-30-7 fills it via QImage bridge.
- **3MF load path** (`ProjectServiceMock.cpp:5110+`) — already extracts `plate->thumbnail_file` and reads pixels via `_extract_from_archive` into `plate_data_list[i]->plate_thumbnail.pixels` (bbs_3mf.cpp:1640). D-30-8's round-trip test verifies this is preserved.
- **`PartPlate` domain** (`PartPlate.h`) — addInstance/removeInstance/clearInstances/setPlateLocked are the content-change hooks (D-30-10 invalidation points).

### Established Patterns
- **Service-as-Qt-adapter** (Phase 16 D-01): ProjectServiceMock exposes PartPlate truth via Q_PROPERTY/Q_INVOKABLE with HAS_LIBSLIC3R guards. New `plateThumbnail`/`generatePlateThumbnailVariant` Q_INVOKABLE follow this.
- **Q_INVOKABLE plate methods emit projectChanged() + plateDataLoaded()** (Phase 16/29 pattern). Thumbnail regeneration on content change emits `plateDataLoaded()`.
- **Test-first on the model** (Phase 16/29): PartPlate unit tests for cache invalidation + ProjectServiceMock integration tests for round-trip. Existing `tests/PartPlateTests.cpp` (created in Phase 29) is the home.

### Integration Points
- **3MF save path** (`store_bbs_3mf` via `buildPlateDataList`) — D-30-5 populates `plate_thumbnail`, and the upstream writer handles the rest (XML metadata + pixel archive write).
- **3MF load path** (ProjectServiceMock.cpp:5110+) — already populates `PlateData::plate_thumbnail.pixels` from the archive; D-30-8 round-trip needs the load path to also write back into `PartPlate::m_thumbnail`.
- **QML plate sidebar** — already binds `generatePlateThumbnail`; Phase 30's variant + data-URL exposure flows through transparently.
- **`PartPlateTests.cpp`** (Phase 29) — add cache-invalidation unit tests here.
- **`auto_verify_with_vcvars.ps1`** — already builds + runs PartPlateTests (Phase 29 added it); Phase 30's new test slots flow through automatically.

</code_context>

<specifics>
## Specific Ideas

- The user's "complete source-truth alignment, no half-measures" pattern held for the variant semantics (mirroring upstream's main/top split) and the persistence mechanism (populating `PlateData::plate_thumbnail` exactly as upstream). The ONE pragmatic divergence is **QImage on PartPlate** (vs upstream's libslic3r ThumbnailData) — this is justified because PartPlate is a Qt-domain object and QImage is QML-native; the conversion to ThumbnailData happens at the save boundary (buildPlateDataList), keeping the domain pure.
- Subtlety: upstream `PlateData::plate_thumbnail` uses `is_valid()` (width>0 && height>0 && !pixels.empty()) as the gate for writing the thumbnail to 3MF (bbs_3mf.cpp:7987). The Qt6 round-trip test must verify `is_valid()` is true after buildPlateDataList populates it.
- The top-down 2D footprint (variant=1) is genuinely new product behavior that doesn't exist in Qt6 today. It must visually communicate "which objects are on this plate and roughly where" — bounding-box rectangles at scaled positions, colored by palette. This is the highest-risk task (new QPainter rendering).
- The save/reload round-trip (THUMB-02) is the single most important test. If the thumbnail doesn't survive save→reload, the persistence is broken. The test should assert: after save+reload, the reloaded plate's thumbnail has the same dimensions as the original and is non-empty.
- Cache invalidation must be wired into ALL content-change paths: addInstance, removeInstance, clearInstances, setPlateLocked (and the existing setOrigin/setSize if those affect appearance — recommend invalidating on all to be safe).

</specifics>

<deferred>
## Deferred Ideas

- **Real GL-capture thumbnails (4 variants)** → v3.3+ (THUMB-03, blocked on QRhi framebuffer; needs actual scene rendering including lighting/no-light/pick variants).
- **`top_file` 3MF persistence for the top variant** → v3.3+ (needs real GL capture; THUMB-03). Phase 30's top variant is UI-only.
- **Object labels in the top-down footprint** → refinement for a future phase (keep Phase 30 minimal).
- **Thumbnail regeneration tied to GL scene dirty events** → Phase 30 auto-regenerates on plate-content change (simpler, no GL coupling); GL-scene-tied regeneration deferred.
- **Animated/live thumbnail updates** → out of scope (static thumbnails only in Phase 30).

</deferred>

---

*Phase: 30-Thumbnail Persistence*
*Context gathered: 2026-06-28 via smart discuss (autonomous mode)*
