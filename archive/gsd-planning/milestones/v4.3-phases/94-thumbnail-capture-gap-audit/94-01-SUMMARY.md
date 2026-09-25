---
phase: 94-thumbnail-capture-gap-audit
plan: 01
subsystem: infra
tags: [thumbnail, qrhi, readback, 3mf, store_bbs_3mf, msaa, render-thread]

# Dependency graph
requires:
  - phase: 93-assembleview-verification-and-cleanup
    provides: v4.2 shipped; default QRhi/D3D11 path owns gizmo/pick/cut/wipe/Preview rendering
provides:
  - Canonical v4.3 thumbnail capture + 3MF writer gap matrix (94-GAP-MATRIX.md) routing Phase 95-98.
  - Frozen THUMBAUDIT-02 decisions: Option A offscreen QRhiTexture readback; single-sample offscreen RT for MSAA; m_fitRequestCount/m_viewPreset synchronize() queue pattern.
  - Exact mock-path + upstream-anchor citations for every THUMB-* region.
affects: [95-qrhi-thumbnail-capture-infrastructure, 96-3mf-thumbnail-write-integration, 97-thumbnail-save-reload-round-trip, 98-thumbnail-verification-and-cleanup]

# Tech tracking
tech-stack:
  added: []
  patterns: [offscreen QRhiTexture render-target + readBackTexture thumbnail capture, render-thread capture queue mirroring synchronize() item->renderer->item pattern]

key-files:
  created:
    - .planning/phases/94-thumbnail-capture-gap-audit/94-GAP-MATRIX.md
    - .planning/phases/94-thumbnail-capture-gap-audit/94-VERIFICATION.md
    - .planning/phases/94-thumbnail-capture-gap-audit/94-01-SUMMARY.md
  modified: []

key-decisions:
  - "QRhi readback approach locked to Option A: offscreen QRhiTexture render-target at thumbnail size + QRhiResourceUpdateBatch::readBackTexture() (mirrors upstream's dedicated thumbnail framebuffer; yields a single-sample thumbnail-sized texture)."
  - "MSAA resolve locked to rendering the thumbnail to a single-sample (sample count 1) offscreen RT so no resolve step is needed at readback (consequence of Option A); on-screen viewport keeps sample count 4 per RhiViewport.cpp:47."
  - "Render-thread capture queue locked to mirror the existing m_fitRequestCount/m_viewPreset synchronize() pattern (RhiViewport.h:314-315, RhiViewport.cpp:415,435,442, RhiViewportRenderer.cpp:35), with readback inside render() and QImage delivered back via a queued signal/connection."
  - "THUMB-READ-SIDE and THUMB-PLATE-CACHE classified preserve (already complete); Phase 97 verifies the round-trip rather than re-implementing."

patterns-established:
  - "Thumbnail capture region matrix modeled on Phase 89 gap matrix structure: 10 THUMB-* regions across a 10-column table plus a Frozen Decisions section."
  - "Freeze-before-implement discipline: the 3 THUMBAUDIT-02 designs are locked here so Phase 95 implements exactly these choices without re-evaluation."

requirements-completed: [THUMBAUDIT-01, THUMBAUDIT-02]

# Metrics
duration: ~15 min
completed: 2026-07-09
---

# Phase 94 Plan 01: Thumbnail Capture Gap Audit Summary

**Canonical v4.3 thumbnail capture + 3MF writer gap matrix with 10 THUMB-* regions and 3 frozen THUMBAUDIT-02 decisions (Option A offscreen QRhiTexture readback, single-sample offscreen RT MSAA resolve, synchronize() render-thread queue)**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-07-09
- **Completed:** 2026-07-09
- **Tasks:** 4
- **Files modified:** 3 (all under `.planning/`)

## Accomplishments

- Created `94-GAP-MATRIX.md` — the canonical routing artifact for Phase 95-98, mapping all 10 thumbnail-capture regions (THUMB-MOCK-GENERATOR through THUMB-CLEANUP) across a 10-column table with exact mock-path + upstream-anchor citations.
- Froze the three THUMBAUDIT-02 designs before implementation: Option A offscreen QRhiTexture render-target + readBackTexture (readback), single-sample offscreen RT (MSAA resolve), and the m_fitRequestCount/m_viewPreset synchronize() queue pattern (render-thread capture queue + QImage callback).
- Confirmed the read side (`ProjectServiceMock.cpp:5455-5466,5654-5660`) and destination cache (`PartPlate.h:122-124,255`) are already complete and classified `preserve`; only the write side + capture infrastructure are new work.
- Routed all 12 v4.3 requirements to their owner phases (95/96/97/98) and confirmed the removed network/device/cloud scope, D3D12, GLGizmoMeasure, auto filament-map, and CLI fixtures stay out of scope.

## Task Commits

Each task was committed atomically:

1. **Task 94-01-01: Create thumbnail capture region matrix skeleton** — `0774e07` (docs)
2. **Task 94-01-02: Map upstream writer anchors and Qt integration points per region** — `d8d9438` (docs)
3. **Task 94-01-03: Freeze QRhi readback, MSAA resolve, and render-thread queue decisions** — `ebb3a53` (docs)
4. **Task 94-01-04: Verify and close the Phase 94 audit plan** — this commit (docs)

## Files Created/Modified

- `.planning/phases/94-thumbnail-capture-gap-audit/94-GAP-MATRIX.md` — the canonical v4.3 thumbnail capture + 3MF writer gap matrix (10 THUMB-* regions, Frozen Decisions section, Requirement Routing, Out-of-Scope Classification, Requirement Coverage, Phase Routing).
- `.planning/phases/94-thumbnail-capture-gap-audit/94-VERIFICATION.md` — Phase 94 source/document verification report (region-ID coverage, column completeness, upstream-anchor citations, frozen decisions, requirement coverage, build decision).
- `.planning/phases/94-thumbnail-capture-gap-audit/94-01-SUMMARY.md` — this summary.

## Decisions Made

- **Option A over Option B for QRhi readback.** Option A (offscreen RT at thumbnail size + readBackTexture) mirrors upstream's dedicated thumbnail framebuffer and yields a single-sample thumbnail-sized texture; the duplicated render pass runs only on rare capture frames (zero steady-state cost). Option B would require both MSAA resolve and downscale.
- **Single-sample offscreen RT for the thumbnail** (sample count 1) as a direct consequence of Option A — eliminates the MSAA resolve step entirely; the on-screen viewport keeps sample count 4 for rendering quality.
- **Render-thread queue mirrors the existing m_fitRequestCount/m_viewPreset synchronize() pattern** so the executor has a concrete, proven template for the GUI->render thread boundary crossing and the QImage callback back to the GUI thread.
- **THUMB-READ-SIDE and THUMB-PLATE-CACHE classified `preserve`** because they are already implemented; Phase 97 verifies the round-trip rather than re-implementing them.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- `94-GAP-MATRIX.md` is the canonical routing artifact; Phase 95 can implement the frozen Option A capture infrastructure (THUMBCAP-01/02/03) without rediscovering the mock paths, upstream anchors, readback decision, MSAA strategy, or queue design.
- Phase 96 can populate `PlateData::plate_thumbnail` + `StoreParams::thumbnail_data` (THUMBWRITE-01/02/03) against the cited `bbs_3mf.cpp:6133,6137,5879` anchors.
- Phase 97 can verify the round-trip against the cited read-side + cache anchors.
- Phase 98 can remove the mock paths and run the canonical verifier + visual evidence.
- No blockers.

## Self-Check: PASSED

- `94-GAP-MATRIX.md`, `94-VERIFICATION.md`, `94-01-SUMMARY.md` exist on disk.
- All 10 THUMB-* region IDs present; all 10 columns present in header order.
- Upstream anchors `bbs_3mf.cpp:6133,6137,5879,1640` cited; mock-path citations `ProjectServiceMock.cpp:4242,4362,4372,5089-5093,5127-5135,5455-5466,5654-5660`, `RhiViewport.cpp:476-488`, `RhiViewport.h:229`, `PartPlate.h:122-124,255` present.
- Three frozen decisions present (readback Option A, MSAA single-sample offscreen RT, render-thread queue).
- THUMBAUDIT-01 and THUMBAUDIT-02 covered.
- `git diff --check` exits 0; encoding guard exits 0 for all three artifacts.
- No production source files modified (docs-only phase).

---
*Phase: 94-thumbnail-capture-gap-audit*
*Completed: 2026-07-09*
