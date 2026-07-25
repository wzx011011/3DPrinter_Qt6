# Phase 214 Plan: Hollow Drain Hole Data and Logic Layer

**Requirement:** HOLLOW-01..03, HOLLOW-06
**Goal:** Add the data + logic layer for the Hollow gizmo (drain hole
place/clear/markers). SLA slicing stays deferred (v5.1+); this edits and
persists holes only.
**Status:** Complete · **Commit:** `59eeffa`

## Files

- Modify: `src/core/services/ProjectServiceMock.h`, `.cpp` (drain hole
  accessors reading `ModelObject::sla_drain_holes`)
- Modify: `src/core/viewmodels/EditorViewModel.h`, `.cpp` (`placeHollowPoint`,
  `hollowMarkerData`, `rebuildHollowMarkerData`, real `deleteSelectedHollowPoints`)

## Data Flow

```
placeHollowPoint(pickedSourceIndex, rayOrigin, rayDir)
  → lazy SceneRaycaster (shared with measure/paint)
  → rebuildWorldTransform (hollowWorldTransform helper)
  → SceneRaycaster::hitTest → { meshLocalPosition (DrainHole::pos),
                                 worldNormal → inverse(worldTransform.linear) }
  → ProjectServiceMock::appendObjectDrainHole (normalized normal)
  → rebuildHollowMarkerData (flatten holes → world-space disc-fan GizmoVertex)
  → emit hollowDataChanged
```

## Steps

- [x] `ProjectServiceMock`: `objectDrainHoleCount`, `objectDrainHoles`
  (QVariantMap list, mesh-local), `appendObjectDrainHole(px..pz, nx..nz,
  radius, height)` with normal normalization, `clearObjectDrainHoles`.
  Includes `libslic3r/SLA/Hollowing.hpp`.
- [x] `EditorViewModel::placeHollowPoint` Q_INVOKABLE — casts the world ray
  through the picked object's volumes via SceneRaycaster; converts world
  normal to mesh-local; appends `sla::DrainHole{pos, normal, radius, height}`.
- [x] `hollowMarkerData` Q_PROPERTY + `hollowDataChanged` signal — packs a
  4-byte uint32 vertex count + N GizmoVertex (7 floats: x,y,z,r,g,b,a) in
  world space (one 16-seg red disc per hole, lifted via object transform).
- [x] `rebuildHollowMarkerData` private helper.
- [x] `deleteSelectedHollowPoints` now clears the object's holes (was TODO);
  also resets `hollowSelectedHoleCount` from `objectDrainHoleCount`.
- [x] `hollowWorldTransform` static helper (Geometry::assemble_transform) for
  the translation/rotation/scale → Transform3d rebuild used by hollow paths
  (avoids a cross-anonymous-namespace forward declaration).

## Verification

- owzx_app_core builds clean.
- Renderer wiring follows in Phase 215.
