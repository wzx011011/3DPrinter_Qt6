#version 440

// O1 (TINT): per-batch uniform-color vertex stage. Same structure as
// model_p3i.vert (volume-local vertices + the per-batch world matrix from
// the BatchBlock UBO, binding 1, selected with a dynamic offset per draw)
// plus the per-batch tint color (binding 2, second dynamic offset). The
// pick thumbnail and the preview ghost shell draw the model VB with this
// stage instead of maintaining recolored vertex duplicates.

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;         // offset 0
  vec4 gizmoCenter; // offset 64 (.xyz center, .w scale) -- unused here
  mat4 view;        // offset 80
};

// Selected per draw via a dynamic offset (one 64B-aligned entry per blob
// batch; identity entries for world-baked scenes make this pipeline usable
// by both modes).
layout(std140, binding = 1) uniform BatchBlock
{
  mat4 batchMatrix;
};

// Selected per draw via a dynamic offset (one slot per blob batch; filled
// from PrepareSceneData::batchPickColors / batchGhostColors).
layout(std140, binding = 2) uniform TintBlock
{
  vec4 tint;
};

layout(location = 0) out vec4 vColor;    // unused by the tint frag
layout(location = 1) out vec3 vWorldPos; // unused by the tint frag
layout(location = 2) out vec4 vTint;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  const vec3 worldPos = (batchMatrix * vec4(position, 1.0)).xyz;
  gl_Position = mvp * vec4(worldPos, 1.0);
  vColor = color;
  vWorldPos = worldPos;
  vTint = tint;
}
