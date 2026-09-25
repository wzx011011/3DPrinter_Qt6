# Phase 112: Per-Volume ITS Accessor And Mesh Cache - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close MEASURE-01: a per-volume ITS (indexed_triangle_set) accessor on `ProjectServiceMock` (current `meshData()` is per-object-flattened). This is the CROSS-WORKSTREAM dependency that unblocks both Phase 113-115 (GLGizmoMeasure raycaster + Measuring) AND the AssembleViewDataPool ModelObjectsClipper (deferred from v4.2).

Pitfalls.md pitfall 6: `Measure::Measuring(const indexed_triangle_set&)` requires a `shared_ptr<ITS>` cache layer with a clear ownership contract. `SurfaceFeature` carries raw `void* volume` / `vector<int>* plane_indices` that must be scrubbed at the boundary.

</domain>

<decisions>
### Carry-Forward (research)
- Current state: `vol->mesh().its` accessed internally at ProjectServiceMock.cpp:339, 563 — but no PUBLIC per-volume ITS accessor. `meshData()` is per-object-flattened.
- Upstream `Measure::Measuring` at Measure.hpp:119-122 constructed from `const indexed_triangle_set&`.
- Upstream `SceneRaycaster::m_volumes` (SceneRaycaster.hpp:71-72) needs one raycaster per volume.
- Pitfall 6: Qt6 codebase has ZERO per-volume ITS exposure today (AssemblyMeasureGeometry.cpp:23,43 "without ITS raycasting"). The ownership contract MUST be the first deliverable.
- v4.2 carry-forward: AssembleViewDataPool ModelObjectsClipper needs per-volume ITS too (bit-4 reservation at AssembleViewDataPool.h).

### Scope
1. **Per-volume ITS accessor** on ProjectServiceMock: `Q_INVOKABLE` or direct API that returns the per-volume `indexed_triangle_set` (or a shared_ptr to it) for a given object+volume index.
2. **Ownership contract:** the ITS lifetime MUST be clear — prefer a `shared_ptr<indexed_triangle_set>` cache that outlives the ModelVolume (mirrors how the AssembleViewDataPool cache works). Document: who owns it, when it's invalidated (model change), how it's keyed (object+volume index).
3. **Mesh cache:** if constructing the ITS per-call is expensive (mesh().its is already in memory via libslic3r, so this may be a shallow-copy/share), add a cache. If shallow-share is safe (libslic3r's TriangleMesh owns the ITS), document that.
4. **SurfaceFeature boundary scrubbing** (pitfall 6): document that raw `void* volume` / `vector<int>* plane_indices` from libslic3r MUST NOT escape into Qt — Phase 113/114 will enforce this when the raycaster + Measuring land.
5. **Regression test + source-audit.**

</decisions>

<code_context>
- `src/core/services/ProjectServiceMock.h` (where the accessor lands)
- `src/core/services/ProjectServiceMock.cpp:339, 563` (existing internal `vol->mesh().its` access — the source)
- `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:119-122` (Measuring ctor — the consumer)
- `third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp:71-72` (the raycaster consumer for Phase 113)
- `src/core/rendering/AssembleViewDataPool.h` (the bit-4 ModelObjectsClipper reservation — cross-workstream consumer)

</code_context>

<deferred>
- Phase 113 (raycaster port) — depends on this accessor.
- Phase 114 (Measuring instantiation) — depends on this accessor.
- AssembleViewDataPool ModelObjectsClipper — also depends (cross-workstream).
</deferred>
