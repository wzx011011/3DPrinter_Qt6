# Phase 129: Paint-Gizmo Gate Fix + Flatten + FixMesh - Context

**Gathered:** 2026-07-15
**Mode:** Auto-generated (discuss skipped)

<domain>
Close POLISH-01/02/03: three small bug fixes from the v4.6 gap analysis.
1. Flip the stale `kViewportTrianglePickingAvailable=false` flag (v4.6 real-ized paint but left the flag false).
2. Wire Flatten to the real `orientObject` (currently mock 6-face).
3. Make fixMesh/reloadFromDisk call real mesh repair (`its_merge_vertices` + `its_remove_degenerate_faces`).
</domain>

<decisions>
- POLISH-01: set `kViewportTrianglePickingAvailable=true` (EditorViewModel.cpp:47). Phase 121-123 shipped real PaintEngine + SceneRaycaster; the flag is stale. canActivateGizmo cases 6/7/10 (Support/Seam/MMU) will then return true when hasSingleObject.
- POLISH-02: flattenSelected (EditorViewModel.cpp:1610) calls `projectService_->orientObject(idx)` (the real `Slic3r::orientation::orient` path at ProjectServiceMock.cpp:2664-2675) instead of the mock 6-hardcoded-face + setObjectRotation. Remove the TODO.
- POLISH-03: fixMesh (ProjectServiceMock.cpp:3847) + reloadFromDisk (:3885) call `its_merge_vertices(vol->mesh().its)` + `its_remove_degenerate_faces(vol->mesh().its)` then `vol->set_mesh(...)` with the repaired mesh. This is the real admesh-layer repair (TriangleMesh.hpp:209/212).
</decisions>
