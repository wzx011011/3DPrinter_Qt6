# Phase 68: Move Gizmo RHI Render - Context

**Gathered:** 2026-07-04
**Mode:** Auto-generated (rendering phase; visual verification deferred to Phase 73)

<domain>
First visible gizmo on the D3D11 RHI path: render the X/Y/Z move arrows when
`gizmoMode == GizmoMove`. Reuses the Phase 66 GizmoGeometry vertex data and
the Phase 67 gizmo state (m_gizmoCenter).

Deliverables:
- New gizmo shaders `rhi_gizmo.vert`/`.frag` (GLSL 440): read extended
  CameraBlock { mat4 mvp; vec3 gizmoCenter; float gizmoScale; }, transform
  `position * gizmoScale + gizmoCenter`, pass through per-vertex color.
- Extend `uploadCameraUniform` to write gizmoCenter (offset 64) +
  gizmoScale (offset 76) into the existing 256-byte uniform buffer.
- New gizmo vertex buffer + 2 gizmo pipelines (lines + triangles) using the
  gizmo shader, same Vertex layout as meshes.
- In `render()`, draw the move gizmo (shaft lines + cone triangles) when
  `m_gizmoMode == GizmoMove` and a selection is present.

Visual verification deferred to Phase 73.
</domain>

<decisions>
## Implementation Decisions

- **Reuse camera uniform buffer (offset packing)**: gizmoCenter at offset 64
  (after the 64-byte MVP), gizmoScale at offset 76. Total 80 bytes << 256.
  Avoids a second uniform buffer + SRB.
- **Separate gizmo pipelines** (not reusing mesh pipelines): the gizmo shader
  applies `position*scale+center` which the mesh shader doesn't. Two pipelines
  (lines for shafts, triangles for cones) mirror the mesh fill/line split.
- **gizmoScale = max(dist*0.15, 5.0)** where dist = length(gizmoCenter - cameraEye).
  Matches the GL path scale formula.
- **GizmoMode check**: only draw when `m_gizmoMode == RhiViewport::GizmoMove` (value 0)
  AND `m_prepareScene.selectedSourceObjectIndex() >= 0`.
- **Depth handling**: gizmo should render ON TOP of geometry (visible through
  objects). Use the no-depth-write pipeline variant (like highlight) so the
  gizmo is always visible. Depth test stays enabled so the gizmo still
  occludes correctly relative to itself.
</decisions>

<deferred>
- Highlight/hover color (Phase 68 TODO from Phase 66) — deferred; all axes
  render with their baked base color.
- Rotate/Scale gizmo rendering -> Phase 70.
- Pick + drag interaction -> Phase 69.
</deferred>
