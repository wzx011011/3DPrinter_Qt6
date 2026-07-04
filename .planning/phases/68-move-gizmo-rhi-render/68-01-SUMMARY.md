---
phase: 68-move-gizmo-rhi-render
plan: 01
status: implemented_pending_visual_verification
requirements_covered: [GMOV-01, GMOV-02]
files_modified:
  - src/qml_gui/Renderer/shaders/rhi_gizmo.vert
  - src/qml_gui/Renderer/shaders/rhi_gizmo.frag
  - src/qml_gui/Renderer/RhiViewportRenderer.h
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - CMakeLists.txt
---

# Phase 68 Plan 01 - Summary

**Status:** Implemented — **visual verification deferred to Phase 73** (blind rendering code).

## What Shipped

### New: `src/qml_gui/Renderer/shaders/rhi_gizmo.{vert,frag}`
GLSL 440 gizmo shaders. The vertex shader reads an extended CameraBlock
`{ mat4 mvp; vec3 gizmoCenter; float gizmoScale; }` at binding 0 and applies
the GL-path displacement: `worldPos = position * gizmoScale + gizmoCenter`.
The fragment shader is a pure color passthrough (per-vertex color from
GizmoGeometry::kAxisColorRGB).

### Updated: `RhiViewportRenderer.{h,cpp}`
- New private members: `m_gizmoLinePipeline`, `m_gizmoTriPipeline`,
  `m_gizmoVertexBuffer`, `m_gizmoVertexBufferUploaded`, `m_gizmoPipelineCreated`,
  `m_gizmoVertexBufferBytes`, `m_cameraEye`.
- New methods: `ensureGizmoPipeline()`, `uploadGizmoBuffer(updates)`,
  `renderMoveGizmo(cb)`.
- `uploadCameraUniform` extended to pack gizmoCenter (offset 64, 12 bytes) +
  gizmoScale (offset 76, 4 bytes) into the existing 256-byte camera uniform
  buffer (total CameraBlock = 80 bytes << 256).
- `uploadCameraUniform` trigger now includes `DirtySelection` so gizmoCenter
  updates when the selection changes (not just on camera move).
- `synchronize()` reads `viewport->m_camera.eye()` into `m_cameraEye` for the
  gizmoScale formula `max(dist*0.15, 5.0)`.
- `render()` calls `uploadGizmoBuffer(updates)` in the scene-upload block and
  `renderMoveGizmo(cb)` after the highlight draw.
- `releaseResources()` resets the gizmo pipelines + buffer + flags.

### Updated: `CMakeLists.txt`
- `qt_add_shaders` block extended with `rhi_gizmo.vert` + `rhi_gizmo.frag`
  (compiled to .qsb at `:/rhi_viewport/shaders/rhi_gizmo.{vert,frag}.qsb`).

## Implementation Approach

- **Uniform buffer sharing**: gizmo reuses the existing camera uniform buffer
  (binding 0) with gizmoCenter/gizmoScale packed after the MVP. The mesh
  shader's CameraBlock only declares `{ mat4 mvp; }` (64 bytes) so it ignores
  the extra data; the gizmo shader declares the full block and reads it.
- **Separate pipelines**: `m_gizmoLinePipeline` (Lines topology for shafts) +
  `m_gizmoTriPipeline` (Triangles topology for cones). Both use no-depth-write
  so the gizmo stays visible through objects (matches GL glClear depth).
- **Per-axis draw calls**: shafts are 2 verts at offsets {0, 38, 76}; cones are
  36 verts at offsets {2, 40, 78}. Three draw calls per topology (matching the
  GL path's per-axis glDrawArrays loop).

## ⚠ Visual Verification Required (Phase 73)

This phase produces on-screen rendering that CANNOT be verified by unit tests
alone. The following MUST be visually confirmed when Phase 73 runs the
canonical build + launches OWzxSlicer.exe:

1. **Gizmo appears when Move mode is active**: import a model, click it, press
   the Move gizmo button → three colored arrows (X=red, Y=green, Z=blue) must
   render at the object's center.
2. **Gizmo scales with camera distance**: orbit/zoom → arrows grow/shrink
   proportionally (gizmoScale formula).
3. **Gizmo renders on top**: arrows visible through the model (no-depth-write).
4. **No regression on Prepare bed grid**: the existing mesh/bed rendering must
   be unaffected by the new uniform-buffer packing (gizmoCenter/Scale are
   ignored by the mesh shader).

## Risk Areas

1. **std140 uniform layout**: gizmoCenter at offset 64 (vec3, 12 bytes) +
   gizmoScale at offset 76 (float, 4 bytes). If the GLSL compiler pads
   differently, the gizmo would render at the wrong position/scale. The
   offsets match the std140 spec (vec3 base alignment 16, but the following
   float can occupy the vec3's tail padding), but this is the highest-risk
   assumption.

2. **Camera-uniform buffer size**: the buffer is 256 bytes; packing 80 bytes
   of data leaves 176 bytes of padding. Safe.

3. **`m_cameraEye` source**: read from `viewport->m_camera.eye()` in
   synchronize. If the CameraController's eye() doesn't reflect the actual
   view position, gizmoScale will be wrong.

## Carry-Forward

- Phase 69 (pick + drag) will use the rendered gizmo + GizmoMath::pickMoveAxis
  (Phase 65) to drive object translation.
- Phase 70 (rotate + scale) will reuse the gizmo pipeline skeleton with
  rotate/scale geometry from Phase 66.
- The `[RHI] gizmo vertex buffer uploaded` and `[RHI] gizmo pipelines created`
  logs confirm the upload/creation paths fire; visual confirmation is the
  remaining gap.
