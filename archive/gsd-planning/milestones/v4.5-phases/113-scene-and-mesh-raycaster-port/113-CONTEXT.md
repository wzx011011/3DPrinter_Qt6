# Phase 113: Scene And Mesh Raycaster Port - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close MEASURE-02: port the ray-triangle intersection math from upstream `MeshRaycaster` (MeshUtils.hpp:159+) into a Qt6 pure-CPU helper. Two-stage pick for mouse-move performance: existing `ObjectPicking::pickSourceObject` (coarse AABB) → new per-triangle ITS raycast on the hit volume only (fine). NOT a full port of upstream `SceneRaycaster` (too wxWidgets/GLVolume-coupled) — port only the math + a thin Qt6 scene wrapper.

</domain>

<decisions>
### Carry-Forward
- Phase 112 (MEASURE-01): per-volume ITS accessor `volumeMeshIts(objectIndex, volumeIndex)` returns `shared_ptr<const indexed_triangle_set>`. This is the input to the per-triangle raycast.
- Existing Qt6 `ObjectPicking::pickSourceObject` (ObjectPicking.h:11) — the coarse AABB stage 1. Phase 113 adds stage 2.
- Upstream `MeshRaycaster` (MeshUtils.hpp:159) — the ray-triangle intersection source. `SceneRaycaster` (SceneRaycaster.hpp) — the scene-level wrapper; too coupled to port directly.
- Research PITFALLS.md pitfall 7: upstream GLGizmoMeasure.cpp:600-615 loops all volumes per mouse-move → Qt6 MUST use two-stage pick + cache Measuring per volume.
- Research ARCHITECTURE.md: port as pure-CPU helpers in `src/core/rendering/` matching the GizmoGeometry/ObjectPicking pattern.

### Scope
1. **MeshRaycaster port** (new `src/core/rendering/MeshRaycaster.h/.cpp`): ray-triangle intersection (Möller–Trumbore or whatever upstream uses) operating on a `shared_ptr<const indexed_triangle_set>` + a transform. Returns hit position + normal + triangle index. Pure CPU; no GL/wxWidgets deps.
2. **SceneRaycaster thin Qt6 wrapper** (new `src/core/rendering/SceneRaycaster.h/.cpp` OR extend ObjectPicking): holds per-volume MeshRaycaster instances (keyed by object+volume index, populated from Phase 112 accessor), provides `hitTest(ray origin/dir, AABB-prefiltered candidate volumes)` that runs the per-triangle raycast on each candidate. Two-stage: stage 1 = ObjectPicking AABB (existing); stage 2 = this.
3. **Two-stage pick integration**: the existing `pickSourceObject` returns the hit object; Phase 113 adds `pickVolumeTriangle(ray, candidateObjects)` that runs the fine raycast on each candidate's volumes, returns the closest hit (volume index + triangle index + position + normal).
4. **Perf**: cache the per-volume MeshRaycaster instances (don't reconstruct per mouse-move). Invalidate on model change.
5. **Regression test + source-audit.**

</decisions>

<code_context>
- `src/core/rendering/ObjectPicking.h:11` (`pickSourceObject` — stage 1, the existing coarse AABB)
- `src/core/services/ProjectServiceMock.h` (Phase 112 `volumeMeshIts` accessor — the input)
- `third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp:159+` (upstream MeshRaycaster — the math source)
- `third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp` (upstream scene wrapper — reference only, too coupled to port)
- `src/core/rendering/GizmoGeometry.h` (the pure-CPU helper pattern to match)

</code_context>

<deferred>
- Phase 114 (Measuring instantiation) — consumes this raycaster.
- Phase 115 (snap UX) — consumes the hit results.
- Full upstream SceneRaycaster EType enum (Bed/Volume/Gizmo) — port only what Qt6 needs (Volume + maybe Gizmo for grabbers).
</deferred>
