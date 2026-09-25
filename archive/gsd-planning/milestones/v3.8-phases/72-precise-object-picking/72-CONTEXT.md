# Phase 72: Precise Object Picking - Context

**Gathered:** 2026-07-04
**Status:** Ready for planning
**Mode:** Auto-approved via `$gsd-autonomous --auto/--all`

<domain>
## Phase Boundary

Replace the default RHI viewport's object-picking approximation with precise
mesh picking. The current RHI path selects objects by projecting each
`PrepareSceneData::ModelBatch` bounding box to a screen-space rectangle; this
can select an object when the pointer is inside the projected AABB but not on
the rendered mesh. The GL reference path already does the desired behavior:
build a screen ray, prefilter by ray-AABB, then test every triangle with the
Moller-Trumbore intersection algorithm and choose the nearest hit.

In scope:
- A pure CPU object-picking helper under `src/core/rendering/` that can be
  tested without QRhi, QQuickItem, or OpenGL.
- Ray-AABB prefiltering and ray-triangle intersection using the already parsed
  `PrepareSceneData::ModelVertex` triangle stream.
- RHI viewport selection and hover using the helper from `pickSourceObjectAt`.
- Preservation of active-plate filtering, source-object index selection, and
  nearest-hit behavior across multiple batches.
- Focused unit tests plus a QML/source audit that rejects the old
  screen-rectangle-only picking path.

Out of scope:
- Gizmo axis picking, already handled by Phases 69 and 70.
- Cut plane and wipe tower rendering, completed in Phase 71.
- Deleting `GLViewportRenderer`, tracked by Phase 73.
- Changing `ProjectServiceMock::meshData()` packing or libslic3r mesh
  semantics.

</domain>

<decisions>
## Implementation Decisions

### Picking Helper Boundary
- Introduce `ObjectPicking` as a static-only pure math/scene helper in
  `src/core/rendering/ObjectPicking.{h,cpp}`.
- Keep `ObjectPicking` free of renderer and QML dependencies. It may depend on
  QtCore/QtGui math types and `PrepareSceneData`, matching the existing
  `GizmoCenter` pattern.
- The public helper should accept a ray origin, normalized ray direction,
  `PrepareSceneData::modelVertices()`, and `PrepareSceneData::modelBatches()`,
  then return the picked source-object index or `-1`.
- `PrepareSceneData` vertices are already in scene/world coordinates after
  `ProjectServiceMock::meshData()` applies instance transforms and placement
  normalization, so the RHI path must not apply an extra model matrix.

### Source-Truth Matching
- Match the GL reference algorithm from
  `src/qml_gui/Renderer/GLViewportRenderer.cpp::pickObject`: ray-AABB
  prefilter first, then Moller-Trumbore triangle intersection, then nearest
  positive hit wins.
- Use an epsilon of `1e-6f` for near-parallel triangle tests and reject hits
  whose ray distance is below `1e-4f`, matching the GL code.
- Ray-AABB must be robust for rays parallel to one or more box axes; a ray
  outside a parallel slab misses, and a ray inside a parallel slab continues.
- Batches with invalid source indices, invalid vertex spans, fewer than three
  vertices, or non-finite coordinates must not be selected.

### RHI Integration
- `RhiViewport::pickSourceObjectAt()` should keep `updatePickingScene()` and
  viewport-size guards, then build the screen ray with `GizmoMath::computeRay`
  and delegate precise hit testing to `ObjectPicking`.
- Remove `projectBoundsToScreenRect` from the RHI picking path. Keeping a stale
  helper is acceptable only if unused by picking and clearly not part of the
  selection algorithm; the preferred outcome is deleting it.
- Keep selection and hover signals unchanged: `objectPickedSource` still emits
  source-object indices, and `hoveredSourceObjectIndex` still updates on
  pointer motion.
- QML remains a thin forwarding layer. It must not contain ray, intersection,
  or triangle picking logic.

### Verification Scope
- Add `ObjectPickingTests` as a standalone QtTest target that compiles
  `ObjectPicking.cpp`, `GizmoMath.cpp`, and `PrepareSceneData.cpp` directly.
- Include a RED test that proves the old RHI behavior is wrong: a ray passes
  through the object's AABB but misses the triangle, so precise picking returns
  `-1`.
- Include nearest-hit and multi-batch/source-index tests so picking keeps GL
  semantics when two triangles overlap the ray at different depths.
- Update `QmlUiAuditTests::rhiViewportSelectionPickingBridgeStaysCppOwned()` so
  it requires `ObjectPicking` usage and rejects `projectBoundsToScreenRect` as
  a picking dependency.

### the agent's Discretion
- Exact helper names inside `ObjectPicking`, provided the public contract is
  deterministic and testable.
- Whether ray-AABB and triangle intersection are public testable functions or
  private helpers covered through `pickSourceObject`.
- Whether `projectBoundsToScreenRect` is deleted or left only as unused legacy
  code, as long as the audit proves `pickSourceObjectAt()` does not use it.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `GizmoMath::computeRay()` already converts screen coordinates into a
  world-space ray from viewport size, projection matrix, and view matrix.
- `PrepareSceneData` already parses packed mesh data into per-batch vertex
  spans, source-object indices, and scene-space bounds.
- `RhiViewport::updatePickingScene()` already mirrors the active plate mesh
  data into `m_pickScene`; this should remain the only scene refresh path for
  object picking.
- `GLViewportRenderer::pickObject()` contains the local GL source-truth bridge
  for ray-AABB plus ray-triangle selection.

### Established Patterns
- Pure math helpers live in `src/core/rendering/` and are tested by standalone
  QtTest targets that compile the implementation file directly
  (`GizmoMathTests`, `GizmoGeometryTests`, `GizmoStateWiringTests`).
- QML ownership boundaries are locked by static source audits in
  `tests/QmlUiAuditTests.cpp`.
- `PrepareSceneDataTests` provides byte-packing helpers and proves mesh batches
  retain active-plate source-object indices and triangle vertex spans.

### Integration Points
- `src/core/rendering/ObjectPicking.h/.cpp`: new pure picking helper.
- `tests/ObjectPickingTests.cpp`: new unit tests for ray-AABB, triangle
  misses, nearest hit, invalid batches, and screen-ray integration.
- `CMakeLists.txt`: register `ObjectPickingTests`.
- `src/qml_gui/Renderer/RhiViewport.h/.cpp`: replace screen-rectangle picking
  with `ObjectPicking`.
- `tests/QmlUiAuditTests.cpp`: update the RHI selection audit to lock precise
  picking and reject old AABB-screen-rectangle selection.

</code_context>

<specifics>
## Specific Ideas

- Do not change `objectPickedSource` or `hoveredSourceObjectIndex` QML-facing
  contracts; only improve hit accuracy behind them.
- Keep all new source comments English and ASCII-only.
- Use only the canonical full verifier for final verification:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Do not create another build directory.

</specifics>

<deferred>
## Deferred Ideas

- Object-picking visual UAT in the running app is folded into Phase 73's final
  RHI-only verification.
- Any GL path deletion or registration cleanup belongs to Phase 73.

</deferred>
