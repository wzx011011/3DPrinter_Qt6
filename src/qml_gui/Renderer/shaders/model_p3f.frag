#version 440

// O1 (TINT): flat vertex-color fill over the model_p3i vertex stage
// (volume-local vertices + per-batch BatchBlock matrix). Serves the
// thumbnail model draw on the indexed path -- the soup path's single
// rhi_viewport fill draw cannot address volume-local vertices.

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 vWorldPos; // unused here

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = vColor;
}
