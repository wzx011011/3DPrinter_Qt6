---
phase: 99-wipe-tower-geometry-gap-audit
plan: 01
subsystem: infra
tags: [wipe-tower, print-readback, rhi, qml, libslic3r, slice-service, geometry]

# Dependency graph
requires:
  - phase: 98-thumbnail-verification-and-cleanup
    provides: v4.3 shipped; default QRhi/D3D11 path owns gizmo/pick/cut/wipe/Preview rendering; GLViewport Q_PROPERTY surface stable
provides:
  - Canonical v4.4 wipe-tower geometry readback + rendering gap matrix (99-GAP-MATRIX.md) routing Phase 100-102.
  - Frozen WTAUDIT-02 decisions: post-slice readback point (SliceService worker Print::wipe_tower_data() capture + sliceFinished GUI-thread delivery); Option A dimensioned-box render-upgrade baseline; data-driven has_wipe_tower() gate.
  - Exact placeholder-path + upstream-anchor citations for every WT-* region.
affects: [100-wipe-tower-geometry-readback, 101-wipe-tower-real-rendering-upgrade, 102-wipe-tower-verification-and-regression]

# Tech tracking
tech-stack:
  added: []
  patterns: [Print::wipe_tower_data() post-slice readback inside SliceService worker with worker-local dim capture, has_wipe_tower() data-driven showWipeTower gate, dimensioned-box render baseline feeding existing buildWipeTowerVertices pipeline]

key-files:
  created:
    - .planning/phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md
    - .planning/phases/99-wipe-tower-geometry-gap-audit/99-VERIFICATION.md
    - .planning/phases/99-wipe-tower-geometry-gap-audit/99-01-SUMMARY.md
  modified: []

key-decisions:
  - "Post-slice readback integration point locked: read Print::wipe_tower_data() inside the SliceService worker after print.process() succeeds (SliceService.cpp:584), capture dims into worker-local storage before the Print is invalidated (activePrint_.store(nullptr) at :625/:629/:634), deliver to the GUI thread alongside sliceFinished (:763), and push into RhiViewport Q_PROPERTYs via EditorViewModel (EditorViewModel.h:860). The Print is only valid mid-slice between activePrint_.store(&print) (:508) and activePrint_.store(nullptr) — no Print* may escape the worker."
  - "Rendering-upgrade locked to Option A as the v4.4 baseline: dimensioned box from real bbx/depth/height/position/width fed to the existing buildWipeTowerVertices (mirrors 3DScene.cpp:840-885 load_wipe_tower_preview make_cube). Option B (real mesh from optional wipe_tower_mesh_data via convex_hull_3d, mirroring 3DScene.cpp:887-925 load_real_wipe_tower_preview) documented as a future upgrade."
  - "has_wipe_tower() gate locked: Print.hpp:988 is the gate; when false, no geometry is pushed and showWipeTower stays false (RhiViewport.h:54/:304, RhiViewportRenderer.h:216) so no placeholder box leaks on single-material slices. SoftwareViewport's differing default show=true (SoftwareViewport.h:231) must be aligned to the same data-driven gate."
  - "PreparePage.qml:1648 GLViewport does NOT bind any wipe-tower Q_PROPERTY today — the unbound-binding surface (renderer falls through to the 10/10/50/100/25 RhiViewport.h:304-309 defaults) is the Phase 100 wiring task, captured under WT-VIEWPORT-DEFAULTS."
  - "Zero references to wipe_tower_data/has_wipe_tower (readback)/get_wipe_tower_depth/get_wipe_tower_bbx/get_rib_offset in src/ today (only has_wipe_tower is a g-code label at EditGCodeDialog.qml:381) — confirms the entire WipeTowerData struct is unused on the Qt side."

patterns-established:
  - "Wipe-tower capture region matrix modeled on Phase 94 gap matrix structure: 8 WT-* regions across a 10-column table plus a Frozen Decisions section."
  - "Freeze-before-implement discipline: the 3 WTAUDIT-02 designs are locked here so Phase 100/101 implement exactly these choices without re-evaluation."
  - "Print-lifetime-bounded readback: capture slice result data into worker-local storage before the Print local is destroyed; never hand a Print* to the GUI thread."

requirements-completed: [WTAUDIT-01, WTAUDIT-02]

# Metrics
duration: ~15 min
completed: 2026-07-11
---

# Phase 99 Plan 01: Wipe-Tower Geometry Gap Audit Summary

**Canonical v4.4 wipe-tower geometry readback + rendering gap matrix with 8 WT-* regions and 3 frozen WTAUDIT-02 decisions (SliceService-worker Print::wipe_tower_data() readback, Option A dimensioned-box render baseline, data-driven has_wipe_tower() gate)**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-07-11
- **Completed:** 2026-07-11
- **Tasks:** 4
- **Files modified:** 3 (all under `.planning/`)

## Accomplishments

- Created `99-GAP-MATRIX.md` — the canonical routing artifact for Phase 100-102, mapping all 8 wipe-tower regions (WT-PLACEHOLDER-BOX through WT-SOFTWARE-VIEWPORT) across a 10-column table with exact placeholder-path + upstream-anchor citations.
- Froze the three WTAUDIT-02 designs before implementation: post-slice readback point (read `wipe_tower_data()` in the SliceService worker between `print.process()` and the `activePrint_.store(nullptr)` invalidation, deliver via `sliceFinished`), Option A dimensioned-box render baseline (Option B real mesh documented as future), and the data-driven `has_wipe_tower()` gate.
- Confirmed the renderer buffer/upload/render pipeline (`RhiViewportRenderer uploadWipeTowerBuffer` + `renderWipeTower` + the `m_wipeTowerDirty` rebuild) is already structurally correct and classified `preserve` for Option A — the gap is the dims it receives, not the buffer path.
- Identified the PreparePage.qml:1648 unbound-binding surface (GLViewport binds no wipeTower Q_PROPERTY today) as the Phase 100 wiring task, and the SoftwareViewport divergent default (`show=true` + zero dims) as an alignment task.
- Routed all 8 v4.4 requirements to their owner phases (100/101/102) and confirmed the removed network/device/cloud scope, D3D12, GLGizmoMeasure, auto filament-map, per-plate wipe-tower refactor, and CLI fixtures stay out of scope.

## Task Commits

Each task was committed atomically:

1. **Task 99-01-01: Create wipe-tower geometry region matrix skeleton** — `3c99424` (docs)
2. **Task 99-01-02: Map upstream Print::wipe_tower_data anchors and Qt integration points per region** — `6a1ed89` (docs)
3. **Task 99-01-03: Freeze readback point, render-upgrade Option A, and has_wipe_tower gate** — `5e1e82d` (docs)
4. **Task 99-01-04: Verify and close the Phase 99 audit plan** — this commit (docs)

## Files Created/Modified

- `.planning/phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md` — the canonical v4.4 wipe-tower geometry readback + rendering gap matrix (8 WT-* regions, Frozen Decisions section, v4.4 Requirement Routing, Out-of-Scope Classification, Requirement Coverage, Phase Routing).
- `.planning/phases/99-wipe-tower-geometry-gap-audit/99-VERIFICATION.md` — Phase 99 source/document verification report (region-ID coverage, column completeness, upstream-anchor citations, frozen decisions, requirement coverage, build decision).
- `.planning/phases/99-wipe-tower-geometry-gap-audit/99-01-SUMMARY.md` — this summary.

## Decisions Made

- **Post-slice readback point = SliceService worker, not a separate service.** The `Print` that owns `wipe_tower_data` is constructed and destroyed entirely inside the SliceService worker lambda; reading it anywhere else would require handing out a dangling pointer. The readback therefore captures plain dim values into worker-local storage and delivers them to the GUI thread via the existing `sliceFinished` queued path, then `EditorViewModel` (which already holds `sliceService_`) pushes them into the `RhiViewport` Q_PROPERTYs.
- **Option A over Option B for the render upgrade.** Option A (dimensioned box) closes the placeholder-defaults gap with minimal risk and reuses the entire existing `buildWipeTowerVertices` + `uploadWipeTowerBuffer` + `renderWipeTower` pipeline unchanged except for the fed dims; it mirrors upstream's default `load_wipe_tower_preview` box path. Option B (real mesh via `wipe_tower_mesh_data`/`convex_hull_3d`) is higher fidelity but needs ITS handling in the renderer and its `wipe_tower_mesh_data` may be `std::nullopt` depending on config — deferred to a future milestone.
- **Data-driven `has_wipe_tower()` gate.** The current structural default (`showWipeTower=false`) already prevents a leak today, but the guarantee must become data-driven once the readback lands so it tracks the real `Print::has_wipe_tower()` result. SoftwareViewport's divergent default (`show=true`) is called out for alignment.
- **WT-RENDERER-BUFFER classified `preserve`.** The buffer/upload/render pipeline is already correct; the `m_wipeTowerDirty` rebuild fires on dim change. No structural change is needed for Option A; the gap is upstream (the dims it receives), not in the buffer path.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- `99-GAP-MATRIX.md` is the canonical routing artifact; Phase 100 can implement the post-slice readback (WTREAD-01), the `has_wipe_tower()` gate (WTREAD-02), the PreparePage.qml wipe-tower Q_PROPERTY bindings, and the SoftwareViewport mirror without rediscovering the placeholder paths, the Print lifetime, or the EditorViewModel bridge.
- Phase 101 can feed the real dims into `buildWipeTowerVertices` (WTRENDER-01) and ship Option A as the v4.4 baseline (WTRENDER-02) against the cited `3DScene.cpp:840-885` (Option A) and `3DScene.cpp:887-925` (Option B) anchors.
- Phase 102 can run the source/QML audits (WTVERIFY-01) and the canonical verifier + runtime wipe-tower visibility evidence (WTVERIFY-02).
- No blockers.

## Self-Check: PASSED

- `99-GAP-MATRIX.md`, `99-VERIFICATION.md`, `99-01-SUMMARY.md` exist on disk.
- All 8 WT-* region IDs present; all 10 columns present in header order.
- Upstream anchors `Print.hpp:740-786,988-989,1078-1080`, `3DScene.cpp:840-885,887-925` cited; placeholder-path citations `GizmoGeometry.h:74`/`.cpp:449-499`, `RhiViewport.h:54-59,181-192,304-309`, `RhiViewportRenderer.h:57,75,129,143,152,177,216-222`/`.cpp:1064-1095,1894-1908`, `SoftwareViewport.h:35-40,126-137,231-236`/`.cpp:207-253`, `PreparePage.qml:1648` present; readback citations `SliceService.cpp:508,584,625/629/634,763`, `EditorViewModel.h:860` present.
- Three frozen decisions present (readback point, render-upgrade Option A, has_wipe_tower gate).
- WTAUDIT-01 and WTAUDIT-02 covered; all 8 v4.4 requirements in the routing table.
- `git diff --check` exits 0; encoding guard exits 0 for all three artifacts.
- No production source files modified (docs-only phase).

---
*Phase: 99-wipe-tower-geometry-gap-audit*
*Completed: 2026-07-11*
