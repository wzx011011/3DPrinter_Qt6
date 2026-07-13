# Phase 115: GLGizmoMeasure Snap UX And Feature Picking - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close MEASURE-04: the GLGizmoMeasure snap UX (Point/Edge/Circle/Plane feature picks; Shift toggles FeatureSelection vs PointSelection) is wired through the Phase 113 raycaster + Phase 114 Measuring. GizmoMeasure=3 enum slot already exists (RhiViewport.h:97). SurfaceFeature boundary scrubbing (pitfall 6) already handled in Phase 114.

This is the most complex WS5 phase — it's the user-facing interaction layer.

</domain>

<decisions>
### Carry-Forward
- Phase 113 (MEASURE-02): SceneRaycaster (two-stage pick, hit result with facetIdx + point + normal).
- Phase 114 (MEASURE-03): MeasureEngine (per-volume Measuring; get_feature returns SurfaceFeature → scrubbed to QtFeature).
- Upstream SurfaceFeatureType enum at Measure.hpp:16-22: Point=1<<0, Edge=1<<1, Circle=1<<2, Plane=1<<3.
- Upstream GLGizmoMeasure.cpp:409-442: Shift toggles FeatureSelection (default, whole feature) vs PointSelection (Shift held, exact point).
- GizmoMeasure=3 enum slot exists (RhiViewport.h:97).
- Research FEATURES.md: 4 feature types; two snap modes (Shift toggle).

### Scope
1. **Mouse-move → raycast → feature pick**: on mouse move in the Prepare/Assembly view with the measure gizmo active, run the two-stage pick (Phase 113 SceneRaycaster) → get the hit (facetIdx + point) → call MeasureEngine::getFeature → get the SurfaceFeature type (Point/Edge/Circle/Plane) + the snapped position.
2. **Shift toggle**: default = FeatureSelection (snap to the nearest feature — edge midpoint, plane, etc.); Shift held = PointSelection (snap to the exact mouse point, no feature). The toggle is read from the keyboard modifiers.
3. **Visual feedback**: highlight the picked feature (the existing gizmo overlay should show what's snapped — a point marker, edge line, circle, or plane). The measurement readout (Phase 114 EditorViewModel Q_PROPERTYs) updates.
4. **Two-point measurement**: clicking sets point A; moving + clicking sets point B; the readout shows the A→B measurement (angle/distance/etc.). Mirrors upstream GLGizmoMeasure.
5. **Performance** (pitfall 7): the two-stage pick + cached Measuring (Phase 113/114) must keep mouse-move responsive. Don't loop all volumes.
6. **Regression test + source-audit.**

</decisions>

<code_context>
- `src/qml_gui/Renderer/RhiViewport.h:97` (GizmoMeasure=3 enum slot — already exists)
- `src/core/rendering/SceneRaycaster.h` (Phase 113 — the pick)
- `src/core/rendering/MeasureEngine.h` (Phase 114 — the feature extraction)
- `src/core/viewmodels/EditorViewModel.h` (Phase 114 readout Q_PROPERTYs — the display)
- `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:16-22` (SurfaceFeatureType)
- `third_party/OrcaSlicer/src/slic3r/GUI/GLGizmoMeasure.cpp:409-442` (upstream Shift toggle)

</code_context>

<deferred>
- MEASURE-05 (regression test + source audit confirming readouts are real) — Phase 116.
- MEASURE-06 (Assembly actions) — LOCKED future.
</deferred>
