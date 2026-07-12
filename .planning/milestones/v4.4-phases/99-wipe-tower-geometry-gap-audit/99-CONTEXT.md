# Phase 99: Wipe-Tower Geometry Gap Audit - Context

**Gathered:** 2026-07-11
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 99 is a read-only audit phase for the v4.4 Wipe-Tower Geometry Readback
And Real Rendering milestone. It freezes the wipe-tower capture + rendering
region map before implementation: mapping the placeholder geometry path, the
upstream `Print::wipe_tower_data()` / `WipeTowerData` anchors, the post-slice
readback integration point, the rendering-upgrade decision (box dims vs real
mesh), and verification expectations.

This phase does not modify production source code. It produces the canonical
v4.4 wipe-tower gap matrix (`99-GAP-MATRIX.md`) that Phase 100-102 execute
against, modeled after the Phase 94/89 gap matrix structure.

In scope:
- Placeholder geometry path: `GizmoGeometry::buildWipeTowerVertices` (36-vertex box), RhiViewport defaults (width=10/depth=10/height=50/x=100/z=25).
- Upstream `Print::wipe_tower_data()` / `WipeTowerData` struct (bbx, depth, height, brim_width, rib_offset, position, width, tool_changes, optional wipe_tower_mesh_data).
- Upstream accessors: `has_wipe_tower()`, `wipe_tower_data()`, `get_wipe_tower_depth()`, `get_wipe_tower_bbx()`, `get_rib_offset()`.
- Upstream GUI rendering reference: `3DScene.cpp:840-882 load_wipe_tower_preview` (make_cube box), `3DScene.cpp:887-923 load_real_wipe_tower_preview` (real mesh via convex_hull_3d).
- The post-slice readback integration point (where Print becomes available after SliceService runs, how it flows to RhiViewport/EditorViewModel).
- The rendering-upgrade decision: dimensioned box from bbx/depth/height (minimum) vs real mesh from wipe_tower_mesh_data (full fidelity, mirrors load_real_wipe_tower_preview).

Out of scope:
- Production code changes.
- Auto filament-map, per-plate wipe-tower architecture refactor, D3D12, GLGizmoMeasure, CLI fixtures.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion (discuss skipped — use ROADMAP goal + exploration findings + codebase conventions)

The gap matrix must capture the following (from pre-planning exploration):

1. **Placeholder paths to replace:**
   - `src/core/rendering/GizmoGeometry.h:74-79` / `.cpp:449-499` — `buildWipeTowerVertices(x, z, width, depth, height)` builds a 36-vertex rectangular prism (6 faces × 2 triangles × 3 verts), hardcoded color `{0.35, 0.60, 0.85, 0.50}`.
   - `src/qml_gui/Renderer/RhiViewport.h:54-59,181-192,304-309` — Q_PROPERTY show/width/depth/height/x/z; defaults width=10, depth=10, height=50, x=100, z=25.
   - `src/qml_gui/Renderer/RhiViewportRenderer.h:57,75,129,143,152,177,216-222` — m_wipeTowerBuffer, m_wipeTowerVertexCount, m_showWipeTower, m_wipeTowerWidth/Depth/Height/X/Z, m_wipeTowerDirty.
   - `RhiViewportRenderer.cpp:1064-1095 uploadWipeTowerBuffer()`, `:1894-1908 renderWipeTower()` (via m_translucentFillPipeline).
   - `src/qml_gui/Renderer/SoftwareViewport.{h,cpp}:35-40,126-127,207-251` — parallel wipe-tower props (no depth/height — fewer props).

2. **Upstream behavior truth anchors (the geometry source):**
   - `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-786` — `struct WipeTowerData`: tool_changes (:754), bbx (:764, includes brim), rib_offset (:765), wipe_tower_mesh_data (:766, optional WipeTowerMeshData), depth, height, brim_width, position, width.
   - `Print.hpp:988-989` — `has_wipe_tower()`, `wipe_tower_data()`.
   - `Print.hpp:1078-1080` — `get_wipe_tower_depth()`, `get_wipe_tower_bbx()`, `get_rib_offset()`.
   - `3DScene.cpp:840-882 load_wipe_tower_preview` — make_cube(width, depth, height), v.is_wipe_tower=true.
   - `3DScene.cpp:887-923 load_real_wipe_tower_preview` — uses mesh.convex_hull_3d() from actual wipe-tower mesh.

3. **Post-slice readback integration point (to be frozen in WTAUDIT-02):**
   - Where does `Print` become available? After `SliceService` runs `print->process()` — check `src/core/services/SliceService.cpp` for the Print object lifetime and how the Qt6 side accesses it (the slicing is libslic3r-backed).
   - How does geometry flow to the renderer? Likely: SliceService/EditorViewModel reads wipe_tower_data after slice → pushes into RhiViewport Q_PROPERTY (wipeTowerWidth/Depth/Height/X/Z) → renderer's uploadWipeTowerBuffer picks up new dims (m_wipeTowerDirty).

4. **Rendering-upgrade decision (to be frozen in WTAUDIT-02):**
   - **Option A (minimum): dimensioned box** — read bbx/depth/height/position/width from wipe_tower_data, push as dims to the existing buildWipeTowerVertices (still a box, but real dimensions). Low risk, fast, mirrors 3DScene.cpp:840 load_wipe_tower_preview.
   - **Option B (full fidelity): real mesh** — read wipe_tower_mesh_data (optional WipeTowerMeshData), extract the indexed_triangle_set, build a real vertex buffer (mirror 3DScene.cpp:887 load_real_wipe_tower_preview via convex_hull_3d). Higher fidelity (shows brim, rib, cone base) but more work + needs ITS handling.
   - Recommendation: start with Option A (dimensioned box from real dims) as the v4.4 baseline since it closes the "placeholder defaults" gap with minimal risk; note Option B as a documented future upgrade.

5. **has_wipe_tower() gating (WTREAD-02):**
   - When `Print::has_wipe_tower()` is false (single-material / enable_prime_tower off), no wipe-tower geometry is pushed and m_showWipeTower stays false — no placeholder box leak.

</decisions>

<code_context>
## Existing Code Insights

### Placeholder paths (to replace)
- `src/core/rendering/GizmoGeometry.h:74-79` / `.cpp:449-499` — buildWipeTowerVertices (box).
- `src/qml_gui/Renderer/RhiViewport.h:54-59,181-192,304-309` — Q_PROPERTY + defaults.
- `src/qml_gui/Renderer/RhiViewportRenderer.{h,cpp}` — m_wipeTowerBuffer, uploadWipeTowerBuffer (:1064-1095), renderWipeTower (:1894-1908).
- `src/qml_gui/Renderer/SoftwareViewport.{h,cpp}:35-40,126-127,207-251` — parallel software renderer props.
- `src/core/viewmodels/PreviewViewModel.cpp:130,156` — {"Prime tower", 13} color table entry.

### Upstream anchors (geometry source)
- `Print.hpp:740-786` (WipeTowerData), `:988-989` (has_wipe_tower/wipe_tower_data), `:1078-1080` (get_wipe_tower_depth/bbx/rib_offset).
- `3DScene.cpp:840-882` (load_wipe_tower_preview), `:887-923` (load_real_wipe_tower_preview).

### Integration point (to map)
- `src/core/services/SliceService.cpp` — where the Print object lives post-slice; the readback must hook here.
- `src/core/viewmodels/EditorViewModel.*` — likely the bridge that pushes geometry to RhiViewport.

### Prior gap-audit pattern (template)
- `.planning/milestones/v4.3-phases/94-thumbnail-capture-gap-audit/94-GAP-MATRIX.md` — the structure to replicate.

</code_context>

<specifics>
## Specific Ideas

- The matrix must explicitly classify the placeholder box as "replace dims" (Option A) and note Option B (real mesh) as a documented future upgrade.
- The post-slice readback integration point must be frozen: confirm SliceService's Print lifetime and the EditorViewModel→RhiViewport push path before Phase 100 implements it.
- The has_wipe_tower() gate must be documented so Phase 100 doesn't leak a placeholder box on single-material slices.

</specifics>

<deferred>
## Deferred Ideas

- Implementation of any readback/rendering code is deferred to Phase 100-102.
- Real-mesh wipe-tower (Option B) is documented as a future upgrade over the Option A dimensioned box.
- Auto filament-map recommendation — future milestone.

</deferred>

---

*Phase: 99-wipe-tower-geometry-gap-audit*
*Context gathered: 2026-07-11 (discuss skipped via workflow.skip_discuss; exploration findings embedded)*
