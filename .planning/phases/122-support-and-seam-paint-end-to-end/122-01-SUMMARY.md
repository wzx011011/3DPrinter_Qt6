# Phase 122+123 Summary: Support/Seam + MMU Paint End-To-End

**Phase:** 122 (PAINT-04) + 123 (PAINT-05) — WS2 complete
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
Closed PAINT-04 (Support/Seam) + PAINT-05 (MMU): painted TriangleSelector writes to ModelVolume FacetsAnnotation (supported_facets/seam_facets/mmu_segmentation_facets) via the ProjectServiceMock bridge, so the slice consumes painted facets. 3MF persistence automatic (bbs_3mf paint_supports/paint_seam/paint_color). WS2 complete (120-123).

## Key mechanism
Paint stored as FacetsAnnotation member (NOT new ModelVolume, NOT PrintConfig). writePaintToModelVolume(obj,vol,PaintKind,selector) calls mv->{supported,seam,mmu}_facets.set(selector). FacetsAnnotation::set serializes + touch() (re-slice). cloneCurrentPlateModel deep-copies FacetsAnnotation → print.apply flows to PrintObject automatically.

## Changes
- PaintEngine.h/.cpp: cachedSelectorForVolume (read-only selector access for write-back).
- ProjectServiceMock.h/.cpp: PaintKind enum + writePaintToModelVolume + clearPaintOnModelVolume (HAS_LIBSLIC3R guarded; writes supported_facets/seam_facets/mmu_segmentation_facets).
- EditorViewModel.h/.cpp: activePaintKind Q_PROPERTY (routes write-back by gizmo); paintAtFacet calls writePaintToModelVolume after paintAt; clearSupportPaintOnSelection/clearSeamPaintOnSelection/clearMmuSegmentation TODO stubs implemented (PaintEngine.clearObject + clearPaintOnModelVolume + loop-counter volume iteration).
- QmlUiAuditTests.cpp: supportAndSeamPaintFeedsSlice + mmuSegmentationPaintFeedsSlice slots.

## Verification
Canonical build (j6) exit 0; all 5 ctest groups PASS; APP_RUNNING_PID=36328.
