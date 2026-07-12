# Phase 109: Option B Wipe-Tower Mesh Readback And Real Rendering - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close WTMESH-01/02/03: when `Print::wipe_tower_data().wipe_tower_mesh_data` is populated (post-slice, multi-material), the renderer draws a real mesh (brim + rib/cone base + true non-convex footprint) via `real_wipe_tower_mesh` + `real_brim_mesh` + `convex_hull_3d`, mirroring upstream `3DScene.cpp:887-925 load_real_wipe_tower_preview`. The v4.4 Option A dimensioned box (Phase 99 Frozen Decision 2) is preserved as fallback when `wipe_tower_mesh_data == std::nullopt`. SoftwareViewport mirrors.

**RE-OPENS Phase 99 Frozen Decision 2** (Option B was LOCKED future upgrade; now implementing).

</domain>

<decisions>
### Carry-Forward (research)
- v4.4 WipeTowerGeometry POD (SliceService.h:42) currently captures only dims (valid/x/z/width/depth/height/brimWidth/ribOffsetX/Y). Option B needs the actual mesh vertices — extend the POD with a `std::vector<float>` meshVertices field (flattened XYZ, captured by value from `real_wipe_tower_mesh.its` + `real_brim_mesh.its` merged + `convex_hull_3d()`).
- Upstream Option B (3DScene.cpp:887-925): `wt_mesh.merge(brim_mesh)` if render_brim, then `mesh.convex_hull_3d()` for the shell. The Qt port should do the same: merge + convex_hull_3d, then extract the ITS vertices.
- WipeTowerMeshData struct (Print.hpp:742-746): `real_wipe_tower_mesh` + `real_brim_mesh` are `TriangleMesh` (have `.its` indexed_triangle_set). `wipe_tower_mesh_data` is `std::optional<WipeTowerMeshData>` (Print.hpp:766) — resets to nullopt on clear() (:781).
- Existing Option A baseline (v4.4): `buildWipeTowerVertices` at GizmoGeometry.cpp:449 (36-vertex box); `uploadWipeTowerBuffer` at RhiViewportRenderer.cpp:1064-1095 (dynamic-size rebuild on m_wipeTowerDirty). Option B is a PARALLEL path — do NOT modify the frozen Option A.

### Scope
1. **Extend WipeTowerGeometry POD** (SliceService.h): add `bool hasRealMesh` + `std::vector<float> meshVertices` (flattened XYZ, convex-hull-of-merged-meshes). Captured by value when `wipe_tower_mesh_data != nullopt`.
2. **Worker capture** (SliceService.cpp): in the existing wipe-tower capture block, when `wipe_tower_mesh_data` is populated, merge `real_wipe_tower_mesh` + `real_brim_mesh`, run `convex_hull_3d()`, extract ITS vertices into `meshVertices`. NO TriangleMesh* escapes — pure float vector.
3. **New buildWipeTowerMeshVertices helper** (GizmoGeometry.h/.cpp): builds a triangle-set vertex buffer from the captured meshVertices (parallel to buildWipeTowerVertices, NOT a modification of it).
4. **uploadWipeTowerBuffer branch** (RhiViewportRenderer.cpp): when `m_wipeTowerHasRealMesh`, call `buildWipeTowerMeshVertices` instead of `buildWipeTowerVertices`. Option A fallback when no mesh.
5. **SoftwareViewport mirror** (SoftwareViewport.cpp): when hasRealMesh, draw the mesh projection (or document deferral — SoftwareViewport is a fallback renderer).
6. **Regression test** (PartPlateTests or ViewModelSmokeTests): assert the Option B path fires only when mesh populated; Option A regression-locked.
7. **Source audit** (QmlUiAuditTests): lock the Option A + Option B coexistence contract.

</decisions>

<code_context>
- `src/core/services/SliceService.h:42-60` (WipeTowerGeometry POD — extend)
- `src/core/services/SliceService.cpp:634-658` (worker capture block — add mesh extraction)
- `src/core/rendering/GizmoGeometry.h:74` + `.cpp:449-533` (buildWipeTowerVertices — Option A, DO NOT MODIFY)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1064-1095` (uploadWipeTowerBuffer — add Option B branch)
- `src/qml_gui/Renderer/RhiViewportRenderer.h:216-222` (m_wipeTowerHasRealMesh + m_wipeTowerMeshVertices members — add)
- `src/qml_gui/Renderer/SoftwareViewport.cpp:447-498` (Phase 101 QPainter box — mirror Option B)
- `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-786` (WipeTowerData + WipeTowerMeshData)
- `third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp:887-925` (load_real_wipe_tower_preview — upstream Option B source-truth)

</code_context>

<deferred>
- WTMESH-04 (regression ctest lock + Option A no-regress proof) — Phase 116 (cross-workstream verification).
- Per-extruder color slabs on the Option B mesh — anti-feature (upstream chose silhouette over stripes).
- Option B in SoftwareViewport may be deferred if QPainter mesh projection is non-trivial; document the gap with justification.
</deferred>
