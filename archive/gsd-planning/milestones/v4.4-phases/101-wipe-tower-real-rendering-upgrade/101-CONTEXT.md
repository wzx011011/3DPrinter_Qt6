# Phase 101: Wipe-Tower Real Rendering Upgrade - Context

**Gathered:** 2026-07-11
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

The rendered wipe-tower reflects real sliced dimensions (not placeholder
10/10/50/100/25). `GizmoGeometry::buildWipeTowerVertices` is upgraded toward
real geometry — dimensioned box minimum (Option A v4.4 baseline), real mesh
(Option B) documented as future upgrade.

Success criteria (from ROADMAP):
1. WTRENDER-01: The rendered wipe-tower reflects the real sliced dimensions
   (width/depth/height/position from `wipe_tower_data`), not the placeholder
   10/10/50/100/25 defaults.
2. WTRENDER-02: `GizmoGeometry::buildWipeTowerVertices` is upgraded toward
   real geometry — Option A (dimensioned box from real bbx/depth/height/
   position/width, fed to the existing box builder) LOCKED as the v4.4
   baseline; Option B (real mesh from `wipe_tower_mesh_data` via
   `convex_hull_3d`) documented as a future upgrade.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user setting. Use the ROADMAP phase goal, Phase 99 gap matrix frozen decisions, and Phase 100's readback output as the source of truth.

### Carry-Forward Inputs (frozen in prior phases)
- Phase 99 (WTAUDIT-02) Frozen Decision 2 — WT-RENDER-UPGRADE: **Option A (dimensioned box from real `bbx`/`depth`/`height`/`position`/`width`, fed to the existing `buildWipeTowerVertices`, mirroring `3DScene.cpp:840-885 load_wipe_tower_preview`) is LOCKED as the v4.4 baseline.** Option B (real mesh from optional `wipe_tower_mesh_data` via `convex_hull_3d`, mirroring `3DScene.cpp:887-925 load_real_wipe_tower_preview`) is documented as a future upgrade. Deviations require re-opening WTAUDIT-02.
- Phase 99 (WTAUDIT-02) Frozen Decision 3 — WT-HAS-WIPE-GATE: `has_wipe_tower()=false` → no geometry pushed, `showWipeTower=false` (no placeholder leak).
- Phase 100 shipped WTREAD-01/02: SliceService captures `WipeTowerGeometry` by value in the worker, delivers via `wipeTowerGeometryReady` signal; EditorViewModel exposes 6 Q_PROPERTYs (showWipeTower, wipeTowerWidth/Depth/Height/X/Z); PreparePage.qml:1648 GLViewport binds all 6 to editorVm; SoftwareViewport default `m_showWipeTower` aligned to false (defensive WTREAD-02). Phase 100 REVIEW W1 fixed: captured x/z now converted corner→center (commit `b12d0e5`) so the rendered box sits on the true position.

### Known Phase 101 Scope (from Phase 99 gap matrix + Phase 100 verification)
1. **WTRENDER-01 (RHI path is already correct end-to-end):** The existing
   `RhiViewportRenderer::uploadWipeTowerBuffer` (RhiViewportRenderer.cpp:1064-1095)
   already rebuilds the box via `buildWipeTowerVertices(m_wipeTowerX, m_wipeTowerZ, m_wipeTowerWidth, m_wipeTowerDepth, m_wipeTowerHeight)` on `m_wipeTowerDirty`, and `synchronize()` (lines 171-189) pulls the real dims from the RhiViewport Q_PROPERTYs (which Phase 100 bound to editorVm). So the RHI path needs NO structural change for Option A — the verification is that the rebuild fires on dim change with the real dims. Phase 101 should add a test/source-audit confirming this, not rewrite the pipeline.

2. **WTRENDER-02 (Option A baseline confirmed, Option B documented as deferred):**
   `buildWipeTowerVertices` (GizmoGeometry.cpp:449-499) already accepts
   width/depth/height and builds a 36-vertex box (Option A structure).
   Phase 101 documents Option A as the v4.4 baseline in a code comment +
   the SUMMARY; Option B (real mesh via `wipe_tower_mesh_data->real_wipe_tower_mesh` + `real_brim_mesh` at Print.hpp:745-746, mirrored from upstream `3DScene.cpp:887-925 load_real_wipe_tower_preview` using `convex_hull_3d()` at :914) is documented as a future upgrade requiring ITS vertex format extension in GizmoGeometry + RhiViewportRenderer. No Option B implementation in Phase 101.

3. **SoftwareViewport rendering gap:** SoftwareViewport.cpp has NO wipe-tower
   paint code (grep returns nothing). The Q_PROPERTY setters exist (lines
   207-253) but no `paint()` consumes them. Phase 101 should EITHER (a) add a
   minimal QPainter wipe-tower box in SoftwareViewport's paint matching the
   RHI path's real dims (mirror of upstream box for the software fallback
   renderer), OR (b) document the gap as accepted for v4.4 (SoftwareViewport
   is registered at main_qml.cpp:305 as the alternate GLViewport QML type but
   is not the active Prepare renderer — the default path is RhiViewport). The
   planner/executor's call based on whether SoftwareViewport is actually
   reachable at runtime.

4. **Optional:** Add a Phase 101 test that feeds real dims through the
   EditorViewModel → RhiViewport Q_PROPERTYs and asserts the renderer's
   `m_wipeTowerDirty` rebuilds with them. OR rely on Phase 102's source audit
   + runtime visual evidence.

</decisions>

<code_context>
## Existing Code Insights

Codebase context will be gathered during plan-phase research. Key anchors:
- `src/core/rendering/GizmoGeometry.h:74` + `.cpp:449-499` (`buildWipeTowerVertices(x, z, width, depth, height)` — Option A box builder; already accepts real dims)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1064-1095` (`uploadWipeTowerBuffer` rebuilds box on `m_wipeTowerDirty`)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:171-189` (`synchronize()` pulls real dims from RhiViewport Q_PROPERTYs)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1894-1908` (`renderWipeTower` draws via `m_translucentFillPipeline`, gated on `m_prepareScene.showBed() && m_showWipeTower`)
- `src/qml_gui/Renderer/SoftwareViewport.cpp` (NO wipe-tower paint code — Q_PROPERTYs exist but unused)
- `src/core/services/SliceService.h` (WipeTowerGeometry POD from Phase 100)
- `src/core/viewmodels/EditorViewModel.h/.cpp` (6 Q_PROPERTYs + onWipeTowerGeometryReady slot from Phase 100)
- `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-786` (WipeTowerData — Option B source for `wipe_tower_mesh_data`)
- `third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp:840-885` (Option A upstream: `load_wipe_tower_preview` + `make_cube`)
- `third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp:887-925` (Option B upstream: `load_real_wipe_tower_preview` + `convex_hull_3d`)

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description, the Phase 99 gap matrix frozen decisions, and Phase 100's readback output.

</specifics>

<deferred>
## Deferred Ideas

- **Option B (real wipe-tower mesh):** the real-mesh upgrade path via `wipe_tower_mesh_data->real_wipe_tower_mesh` + `real_brim_mesh` (Print.hpp:745-746) using `convex_hull_3d()` (mirroring upstream `3DScene.cpp:887-925 load_real_wipe_tower_preview`). Requires ITS vertex format extension in `GizmoGeometry` + `RhiViewportRenderer`. LOCKED as future upgrade per Frozen Decision 2; NOT in Phase 101 scope.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).
- D3D12/Vulkan backend work.
- Auto filament-map, CLI fixture, GLGizmoMeasure-engine work.
- libslic3r slicing algorithm changes.

</deferred>
