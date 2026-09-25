# Phase 120: TriangleSelector Engine Port - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close PAINT-01: port the upstream TriangleSelector triangle-pick + adaptive-subdivide + paint-state pipeline to the Qt6 C++ layer, so mouse input can be resolved to a specific triangle index on the selected object's mesh and paint states applied with adaptive subdivision. This is the foundation that unblocks PAINT-02/03 (overlay + brush, Phase 121), PAINT-04 (Support/Seam, Phase 122), and PAINT-05 (MMU, Phase 123).

**Key finding: TriangleSelector is already compiled into the project** (CMakeLists.txt:457-458, .obj exists). It is pure libslic3r code with no GL/wx coupling — reused byte-for-byte. Phase 120 builds the Qt6 owner/wrapper, NOT a reimplementation.

</domain>

<decisions>
## Implementation Decisions (from code research)

### Reuse upstream TriangleSelector directly
`Slic3r::TriangleSelector` (`third_party/OrcaSlicer/src/libslic3r/TriangleSelector.hpp:299`) is pure libslic3r. The GUI subclasses (TriangleSelectorGUI/Patch/GLGizmoPainterBase) are GL/wx-coupled and NOT reused. Phase 120 builds a Qt6 equivalent: a per-volume owner that constructs TriangleSelector + drives select_patch + reads get_facets.

### Three structural gaps to bridge
1. **TriangleSelector ctor takes `const TriangleMesh&`, but ProjectServiceMock exposes `shared_ptr<const indexed_triangle_set>`.** Add a `volumeMeshTriangleMesh(obj, vol)` accessor returning `shared_ptr<const TriangleMesh>` via the same aliasing-construct shallow-share pattern (ProjectServiceMock.h:401-430). TriangleSelector holds a `const TriangleMesh& m_mesh` reference (TriangleSelector.hpp:477) — the TriangleMesh must outlive the selector; the shared_ptr guarantees that.
2. **SceneRaycasterHit drops the mesh-local hit position.** TriangleSelector::select_patch needs mesh-local `hit` + `facet_start`. Add a `meshLocalPosition` field to SceneRaycasterHit (MeshRaycasterHit already has it at MeshRaycaster.h:52; it's discarded at SceneRaycaster.cpp:85-89). Alternatively, Phase 120's paint-pick calls MeshRaycaster directly. Preferred: extend SceneRaycasterHit (minimal, reusable).
3. **m_paintData is a flat placeholder.** `EditorViewModel::m_paintData` (QList<ObjectPaintData>, EditorViewModel.h:1151) is a flat per-triangle map with no subdivision. Replace/supplement with `std::vector<std::unique_ptr<Slic3r::TriangleSelector>>` keyed per-volume (mirror upstream GLGizmoPainterBase::m_triangle_selectors at GLGizmoPainterBase.hpp:250).

### Paint-pick path (reuses v4.5 raycast infrastructure)
- Stage 1 (object pre-filter): reuse `RhiViewport::pickSourceObjectAt` (RhiViewport.cpp:850-868).
- Stage 2 (triangle facet): reuse `SceneRaycaster::hitTest` (returns facetIdx + worldPosition; extend to carry meshLocalPosition).
- TriangleSelector call: `select_patch(facet_start=facetIdx, cursor, new_state, trafo_no_translate, triangle_splitting=true)`.

### Cursor types (reuse upstream)
Upstream `Cursor` subclasses (Sphere/Circle/HeightRange/Capsule3D/Capsule2D, TriangleSelector.hpp:73-238) are pure math — reuse directly. The brush UI (size, type) is Phase 121 (PAINT-03).

### Unit-testable boundary
Extract a pure helper for the "given a hit + facet + brush params → the selector state change" so it can be unit-tested without a Model/renderer (mirrors v4.5 Measure-engine pattern + Phase 118 convertTicksToCustomGcodeInfo).

</decisions>

<specifics>
## Code Access Points
- SceneRaycaster: `src/core/rendering/SceneRaycaster.h:64-72` (SceneRaycasterHit), `:42-98` (hitTest). MeshRaycaster: `src/core/rendering/MeshRaycaster.h:49-55` (MeshRaycasterHit has meshLocal position).
- Per-volume ITS: `ProjectServiceMock.h:462` (volumeMeshIts). Add volumeMeshTriangleMesh here.
- EditorViewModel paint: `EditorViewModel.h:264` (setTriangleSupportState Q_INVOKABLE), `:1151` (m_paintData — replace). `EditorViewModel.cpp:1012-1030` (current stub).
- SupportPaintTypes.h: ObjectPaintData/TrianglePaintData/SupportPaintState (data layer; Phase 120 may keep as the Qt-side enum mirror of EnforcerBlockerType).
- RhiViewport pick: `RhiViewport.cpp:850-868` (stage 1), `:929-959` (emitMeasurePickIfActive template — add a parallel paint pick).

## Source-Truth Anchors
- TriangleSelector: `third_party/OrcaSlicer/src/libslic3r/TriangleSelector.hpp:299,306-312,333,357,360` (ctor/select_patch/get_facets/serialize/deserialize).
- EnforcerBlockerType: `TriangleSelector.hpp:13-38` (None/Enforcer/Blocker + Extruder1..16 for MMU).
- Cursor: `TriangleSelector.hpp:73-238` (Sphere/Circle/etc + cursor_factory).
- GLGizmoPainterBase (Qt reference, NOT reused): `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoPainterBase.hpp:183-342` (per-volume selector ownership, gizmo_event pattern).

</specifics>

<deferred>
## Deferred Ideas
- Overlay RENDERING (colored-facet overlay on QRhi) → Phase 121 (PAINT-02).
- Brush interaction UI (size slider, type toggle, smart-fill) → Phase 121 (PAINT-03).
- Support/Seam end-to-end slice integration → Phase 122 (PAINT-04).
- MMU segmentation → Phase 123 (PAINT-05).
- 3MF persistence of paint data (serialize/deserialize wiring) → Phase 122/123 (the TriangleSelector serialize API exists; wiring to 3MF save/load is part of the end-to-end phases).

</deferred>
