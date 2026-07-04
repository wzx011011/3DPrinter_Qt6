# Phase 71: Cut Plane + Wipe Tower - Context

**Gathered:** 2026-07-04
**Status:** Ready for planning
**Mode:** Auto-approved via `$gsd-autonomous --auto/--all`

<domain>
## Phase Boundary

Port the cut plane gizmo and wipe tower rendering onto the default D3D11 RHI
path. This phase closes the remaining non-picking rendering parity gap between
`GLViewportRenderer` and `RhiViewportRenderer`: cut mode shows a translucent
axis-aligned cut quad with an outline that tracks `cutAxis` and `cutPosition`,
and the wipe tower appears as a translucent bed-mounted box when the scene
contains wipe tower properties.

In scope:
- Pure CPU geometry builders for cut plane fill/outline and wipe tower box
  vertices, so the rendering geometry can be tested without GL/RHI.
- `RhiViewportRenderer::synchronize()` reading all wipe tower properties from
  `RhiViewport`, with dirty tracking when any property changes.
- QRhi vertex buffers and draw calls for cut fill, cut outline, and wipe tower
  fill using the existing `GizmoVertex` position/color layout.
- Transparent/no-depth-write pipelines for the semi-transparent surfaces,
  plus a line pipeline for the cut-plane outline.
- Thin `PreparePage.qml` bindings for cut and wipe tower properties. QML may
  bind values but must not build geometry or decide render semantics.
- Focused geometry/unit audits plus the canonical verifier.

Out of scope:
- Precise ray-triangle object picking, which remains Phase 72.
- Deleting GLViewportRenderer, which remains Phase 73.
- Full upstream Cut gizmo UI panels beyond the already-existing `cutAxis` and
  `cutPosition` controls.
- Interactive cut-plane dragging in the viewport. The ROADMAP asks for
  cutAxis/cutPosition interaction; existing controls already mutate the
  viewmodel, and this phase makes the RHI plane follow those values.

</domain>

<decisions>
## Implementation Decisions

### Cut Plane Rendering
- Follow the current GL path source truth: render cut plane only for gizmo modes
  Cut (5) and AdvancedCut (14), and only when a source object is selected.
- Build the plane from the selected source object's model-batch bounding box.
  The cut axis is clamped to X/Y/Z, the plane coordinate is `cutPosition`, and
  the two non-cut axes are expanded by five percent for visibility.
- Use axis colors matching the GL cut plane semantics: X red, Y green, Z blue;
  fill alpha is 0.30 and outline alpha is 0.90.
- Generate fill as two triangles and outline as line segments. The GL path uses
  a triangle strip and line strip, but RHI triangle/list topology keeps upload
  and draw calls consistent with existing renderer pipelines.

### Wipe Tower Rendering
- Follow the current GL path source truth: render a rectangular prism on the
  bed only when bed rendering is enabled and `showWipeTower` is true.
- Treat `wipeTowerX`/`wipeTowerZ` as the box center on the bed plane; width and
  depth are symmetric around that center; height extends upward from the bed.
- Use the GL path's bed contact y value `-0.04f`, 36 triangle vertices, and
  semi-transparent accent color `{0.35, 0.60, 0.85, 0.50}`.
- Invalid or non-positive dimensions produce no wipe tower vertices.

### RHI Integration
- Reuse the existing `GizmoVertex` layout and mesh shaders for cut/wipe
  vertices. No new shader should be required.
- Add a transparent fill pipeline with depth test on, depth write off, and
  source-alpha blending enabled. Keep the cut outline as a separate line draw.
- Upload cut and wipe buffers before `beginPass()` whenever selection, mesh,
  cut parameters, or wipe tower properties change.
- Keep the existing move/rotate/scale gizmo pipeline unchanged. Cut and wipe
  are scene-space geometry, not scaled gizmo geometry.

### Verification Scope
- Extend `GizmoGeometryTests` with deterministic geometry tests for cut plane
  fill/outline and wipe tower vertices.
- Extend `QmlUiAuditTests` with a C++ ownership audit for RHI cut/wipe
  rendering, synchronization, transparent pipeline setup, and thin QML binding.
- Run focused targets first, then the canonical build script before marking
  the phase complete.

### the agent's Discretion
- Exact private helper names in `RhiViewportRenderer`.
- Whether cut and wipe buffers share one upload helper or have separate helpers,
  provided the draw calls remain clear and dirty tracking is correct.
- Whether the cut outline uses a dedicated transparent line pipeline or the
  existing line pipeline, provided the outline is visible and separate from
  the fill.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/rendering/GizmoGeometry.{h,cpp}` already owns pure CPU geometry for
  move, rotate, and scale gizmos and is tested by `tests/GizmoGeometryTests.cpp`.
- `GizmoVertex` is the shared `{x,y,z,r,g,b,a}` layout consumed by
  `RhiViewportRenderer`.
- `src/qml_gui/Renderer/GLViewportRenderer.cpp` already implements
  `renderCutPlane`, `buildWipeTowerGeometry`, and `renderWipeTower`; these are
  the local source-truth bridge for this phase.
- `src/qml_gui/Renderer/RhiViewport.h/.cpp` already exposes `showWipeTower`,
  `wipeTowerWidth`, `wipeTowerDepth`, `wipeTowerHeight`, `wipeTowerX`,
  `wipeTowerZ`, `cutAxis`, and `cutPosition`, with setters that call `update()`.
- `src/qml_gui/pages/PreparePage.qml` already binds `cutAxis` and
  `cutPosition` into `GLViewport`/`RhiViewport`.

### Established Patterns
- RHI scene geometry uploads happen before `beginPass()` through
  `QRhiResourceUpdateBatch`.
- `RhiViewportRenderer::synchronize()` mirrors QML item state into renderer
  members, then `render()` branches on `m_canvasType` and current dirty flags.
- GPU buffers are owned as `std::unique_ptr<QRhiBuffer>` with byte-size fields,
  draw-count fields, and helper upload methods.
- Static UI ownership tests in `QmlUiAuditTests.cpp` prevent QML from taking
  over renderer logic.

### Integration Points
- `src/core/rendering/GizmoGeometry.h/.cpp`: add `buildCutPlaneVertices`,
  `buildCutPlaneOutlineVertices`, and `buildWipeTowerVertices`.
- `tests/GizmoGeometryTests.cpp`: add RED tests for cut plane orientation,
  colors, counts, bbox expansion, and wipe tower box dimensions.
- `src/qml_gui/Renderer/RhiViewportRenderer.h/.cpp`: add cut/wipe buffers,
  dirty tracking, upload helpers, transparent pipeline creation, synchronize
  property reads, and draw calls.
- `src/qml_gui/pages/PreparePage.qml`: ensure RHI viewport receives wipe tower
  properties as thin bindings if the viewmodel exposes them, without embedding
  geometry logic.
- `tests/QmlUiAuditTests.cpp`: add a source audit that pins the C++ ownership
  boundary for cut/wipe rendering.

</code_context>

<specifics>
## Specific Ideas

- Use GL path line references as source truth:
  `GLViewportRenderer::renderCutPlane`, `buildWipeTowerGeometry`, and
  `renderWipeTower`.
- Preserve the existing `build/` directory and canonical build command only:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Do not edit `third_party/OrcaSlicer` for this phase.

</specifics>

<deferred>
## Deferred Ideas

- Viewport drag handles for moving the cut plane directly.
- Ray-triangle picking for object selection, tracked by Phase 72.
- Removal of the GL path, tracked by Phase 73.

</deferred>
