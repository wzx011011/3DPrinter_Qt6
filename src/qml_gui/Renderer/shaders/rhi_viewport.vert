#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;
};

layout(location = 0) out vec4 vColor;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  gl_Position = mvp * vec4(position, 1.0);
  vColor = color;
}
