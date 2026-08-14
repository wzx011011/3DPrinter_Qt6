#version 440

// v5.15 (BEDTEX): textured print-bed quad. Mirrors upstream PartPlate
// "printbed" shader contract (position + uv, camera mvp) — see
// third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp render_logo_texture.

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;
  vec4 gizmoCenter;
  mat4 view;
};

layout(location = 0) out vec2 vUv;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  gl_Position = mvp * vec4(position, 1.0);
  vUv = uv;
}
