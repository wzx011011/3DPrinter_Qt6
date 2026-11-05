#version 440

// FIX5M (P3 derivative lighting): position+color only -- no normal attribute,
// no per-vertex lighting. The fragment stage reconstructs the per-face normal
// from screen-space derivatives (normalize(cross(dFdx, dFdy))), which gives
// flat shading identical to the CPU per-face normal bake while removing the
// 12B/vertex normal attribute and the per-upload normal recompute entirely.
// Tracked deviation (same as model_lit.vert): two-sided lighting in the
// fragment stage (max(+dot, -dot)) because the CPU bake applies a pure Y/Z
// axis swap that cannot guarantee outward-facing normals.

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;         // offset 0
  vec4 gizmoCenter; // offset 64 (.xyz center, .w scale) -- unused here
  mat4 view;        // offset 80
};

layout(location = 0) out vec4 vColor;    // raw baked uniform_color (composition in frag)
layout(location = 1) out vec3 vWorldPos; // for the derivative normal + print-volume test

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  gl_Position = mvp * vec4(position, 1.0);
  vColor = color;
  vWorldPos = position;
}
