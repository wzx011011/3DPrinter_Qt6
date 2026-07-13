# Phase 114: Measure Engine Instantiation And Feature Readouts - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close MEASURE-03: instantiate `Measure::Measuring` per-volume (NOT reimplement the math) to produce real measurements (angle, direct/perpendicular distance, distance XYZ), replacing the current AABB stub at AssemblyMeasureGeometry. Per-volume Measuring cached alongside the Phase 113 raycaster.

Pitfall 6: `SurfaceFeature` carries raw `void* volume` / `vector<int>* plane_indices` that MUST be scrubbed at the libslic3r→Qt boundary.

</domain>

<decisions>
### Carry-Forward
- Phase 112 (MEASURE-01): per-volume ITS accessor (`volumeMeshIts`).
- Phase 113 (MEASURE-02): SceneRaycaster + MeshRaycaster (per-volume raycast; hit result includes facetIdx).
- Upstream `Measure::Measuring` at Measure.hpp:119-145: `explicit Measuring(const indexed_triangle_set&)`; `get_feature(face_idx, point, world_tran, only_select_plane)` returns `optional<SurfaceFeature>`.
- Current Qt6 AABB stub: AssemblyMeasureGeometry (center-to-center distance + longest-axis angle) — the v4.2 placeholder.
- Research FEATURES.md: produces Angle, Direct/Perpendicular Distance, Distance XYZ (upstream GLGizmoMeasure.cpp:1990-2048).
- Pitfall 6: SurfaceFeature raw pointers — scrub at boundary.

### Scope
1. **Per-volume Measuring cache** (extend SceneRaycaster or a new MeasureEngine helper): instantiate `Measure::Measuring` per-volume from the Phase 112 ITS, cache alongside the raycaster, invalidate on model change.
2. **get_feature wiring**: when the raycaster returns a hit (facetIdx + point), call `Measuring::get_feature(face_idx, point, world_tran, only_select_plane)` to get the SurfaceFeature. Extract the measurement (angle / distance / etc.).
3. **SurfaceFeature boundary scrubbing** (pitfall 6): the SurfaceFeature from libslic3r carries raw `void* volume` / `vector<int>* plane_indices`. Qt6 must extract the measurement VALUES (angle, distance, position, type) into Qt-owned types BEFORE the SurfaceFeature goes out of scope. No raw libslic3r pointers escape into Qt.
4. **Measurement readout API**: expose the real measurements on EditorViewModel (or a new MeasureViewModel) for Phase 115 UI binding: angle, direct distance, perpendicular distance, distance XYZ.
5. **AABB stub replacement**: AssemblyMeasureGeometry either replaced or augmented with the real Measuring path. Document the relationship.
6. **Regression test + source-audit.**

</decisions>

<code_context>
- `src/core/rendering/SceneRaycaster.h` (Phase 113 — the raycaster hit feeds get_feature)
- `src/core/services/ProjectServiceMock.h` (Phase 112 volumeMeshIts)
- `src/core/rendering/AssemblyMeasureGeometry.h/.cpp` (the AABB stub to replace/augment)
- `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:119-200` (Measuring + SurfaceFeature + DistAndPoints + AngleAndEdges)
- `third_party/OrcaSlicer/src/slic3r/GUI/GLGizmoMeasure.cpp:1990-2048` (upstream measurement readout reference)

</code_context>

<deferred>
- Phase 115 (snap UX: Point/Edge/Circle/Plane feature picks + Shift toggle) — consumes the measurement readouts.
- Assembly-mode transformation actions (MEASURE-06) — LOCKED future.
</deferred>
