#version 440

// Phase 68: gizmo vertex shader. Same vertex layout as meshes (location 0 =
// position, location 1 = color) but applies gizmo displacement:
//   world_pos = aPos * uGizmoScale + uGizmoCenter
// The unit-space gizmo vertices from GizmoGeometry (origin..1.0 range) are
// scaled by the camera-distance-derived uGizmoScale and translated to the
// selected object's AABB center. Matches the GL path's uGizmoCenter/uGizmoScale
// uniforms (GLViewportRenderer.cpp kGizmoVertSrc).
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;
  vec3 gizmoCenter;
  float gizmoScale;
};

layout(location = 0) out vec4 vColor;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  vec3 worldPos = position * gizmoScale + gizmoCenter;
  gl_Position = mvp * vec4(worldPos, 1.0);
  vColor = color;
}
