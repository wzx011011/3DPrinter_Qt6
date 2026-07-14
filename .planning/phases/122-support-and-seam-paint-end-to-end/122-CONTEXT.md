# Phase 122: Support And Seam Paint End-To-End - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close PAINT-04: Support/Seam paint feeds the slice as modifiers and affects support/seam placement. Phase 120 built PaintEngine; Phase 121 renders overlay; Phase 122 writes PaintEngine → ModelVolume FacetsAnnotation → slice.

</domain>

<decisions>
## Implementation Decisions (from research)

### Paint stored as FacetsAnnotation member (NOT new ModelVolume, NOT PrintConfig)
The upstream mechanism: each ModelPart volume has 3 FacetsAnnotation members — `supported_facets` (Model.hpp:870), `seam_facets` (:873), `mmu_segmentation_facets` (:876). Phase 122 writes the PaintEngine selector into supported_facets/seam_facets. NOT creating SupportEnforcer/SupportBlocker ModelVolume (that's a separate geometric-modifier flow).

### Single bridge API
Mirror upstream `update_model_object()` (GLGizmoFdmSupports.cpp:577, GLGizmoSeam.cpp:315): a ProjectServiceMock accessor `writePaintToModelVolume(obj, vol, paintKind, const TriangleSelector&)` that calls `mv->supported_facets.set(selector)` / `mv->seam_facets.set(selector)`. FacetsAnnotation::set (Model.cpp:3524-3533) calls selector.serialize() + touch() (timestamp bump → re-slice).

### Slice flow is automatic
cloneCurrentPlateModel deep-copies FacetsAnnotation (Model.hpp:1111). print.apply(*modelForSlice, config) flows them to PrintObject::project_and_append_custom_facets (PrintObject.cpp:4646-4669) → SeamPlacer + support generators. No SliceService change.

### 3MF persistence is automatic
bbs_3mf writes paint_supports/paint_seam attributes (bbs_3mf.cpp:328-331); load restores them. No 3MF layer change once FacetsAnnotation is populated.

### Wire into paintAtFacet
EditorViewModel::paintAtFacet (Phase 120) currently drives PaintEngine + updates m_paintData placeholder but never writes back to ModelVolume::*_facets. Phase 122 adds the write-back (writePaintToModelVolume) + slice invalidation. Support needs enable_support in config (Print.cpp:1622 warns otherwise).

### Clear stubs
clearSupportPaintOnSelection (EditorViewModel.cpp:985 TODO), clearSeamPaintOnSelection (:1063 TODO) → implement: PaintEngine.clearObject + FacetsAnnotation.reset() (Model.cpp:3535) + invalidate slice.

</decisions>

<specifics>
## Code Access Points
- ProjectServiceMock.h/.cpp: add writePaintToModelVolume + clearPaintOnModelVolume (paintKind enum: Support/Seam/MMU).
- EditorViewModel.cpp:2519-2631 (paintAtFacet — add write-back after PaintEngine.paintAt), :985/:1063 (clear stubs).
- Model.hpp:870/873/876 (FacetsAnnotation members), Model.cpp:3524-3540 (set/reset).
- TriangleSelector.hpp:357/360 (serialize/deserialize — FacetsAnnotation::set uses serialize internally).
- PreparePage.qml: support/seam panel clear buttons (wire to the now-implemented clear methods).

## Source-Truth Anchors
- Upstream update_model_object: GLGizmoFdmSupports.cpp:568-589, GLGizmoSeam.cpp:306-323.
- FacetsAnnotation::set: Model.cpp:3524-3533.
- PrintObject consumption: PrintObject.cpp:4646-4669.

</specifics>

<deferred>
## Deferred Ideas
None — Phase 122 closes Support/Seam paint. MMU → Phase 123.

</deferred>
