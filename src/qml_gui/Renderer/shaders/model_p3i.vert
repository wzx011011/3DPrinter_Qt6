#version 440

// FIX5M-B3 (step 3): indexed pipeline variant of model_p3 -- identical
// lighting contract (fragment-stage derivative normals, see model_p3.frag),
// but the vertices are VOLUME-LOCAL: the per-batch world matrix comes from
// the BatchBlock UBO selected with a dynamic offset per draw (upstream
// GLVolume "matrix is the transform"). A drag updates one 64B uniform entry
// instead of rewriting vertex data.

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

layout(location = 0) out vec4 vColor;    // raw baked uniform_color (composition in frag)
layout(location = 1) out vec3 vWorldPos; // for the derivative normal + print-volume test

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
}
