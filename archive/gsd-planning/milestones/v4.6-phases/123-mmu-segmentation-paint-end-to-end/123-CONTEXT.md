# Phase 123: MMU Segmentation Paint End-To-End - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close PAINT-05: MMU segmentation paint feeds the multi-material slice. Same FacetsAnnotation bridge as Phase 122 (supported/seam), but mmu_segmentation_facets + Extruder1..16 states.

</domain>

<decisions>
## Implementation Decisions (from research)

### Same bridge as Phase 122, different FacetsAnnotation member
Phase 122's writePaintToModelVolume(obj,vol,PaintKind::Mmu,selector) writes `mv->mmu_segmentation_facets.set(selector)`. The selector carries Extruder1..16 states (TriangleSelector.hpp:13-38). Phase 123 wires the MMU gizmo (gizmoMode==10) paint path to PaintKind::Mmu.

### Slice consumption
libslic3r multi_material_segmentation_by_painting (MultiMaterialSegmentation.cpp:2197) reads mmu_segmentation_facets via PrintObjectSlice.cpp:877. Requires multi-material active (multiple extruders) or the paint has no effect.

### Honest multi-material-only behavior
When the project is single-extruder, the MMU gizmo surfaces an honest "requires multi-material" reason (no silent no-op). The existing mmuSelectedExtruder/mmuExtruderCount Q_PROPERTYs drive this.

### Clear stub
clearMmuSegmentation (EditorViewModel.cpp:1234 TODO return false) → implement: PaintEngine.clearObject + clearPaintOnModelVolume(Mmu) + invalidate + return true.

</decisions>

<specifics>
## Code Access Points
- ProjectServiceMock writePaintToModelVolume(Phase 122 added) — reuse with PaintKind::Mmu.
- EditorViewModel.cpp:2519-2631 (paintAtFacet — wire gizmoMode==10 → PaintKind::Mmu), :1234 (clearMmuSegmentation TODO).
- Model.hpp:876 (mmu_segmentation_facets).
- MultiMaterialSegmentation.cpp:2197 (slice consumption).
- PreparePage.qml MMU panel (:2840-2927) — honest gating.

## Source-Truth Anchors
- Upstream: GLGizmoMmuSegmentation.cpp:691-700 (update_model_object for mmu).
- MultiMaterialSegmentation.cpp:2197 (consumption).

</specifics>

<deferred>
## Deferred Ideas
None — Phase 123 closes WS2 (with 120-122).

</deferred>
