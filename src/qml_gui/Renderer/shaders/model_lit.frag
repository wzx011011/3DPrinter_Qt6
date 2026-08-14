#version 440

// v5.15 (MODELLIT): passthrough — lighting is computed per vertex (upstream
// gouraud pattern); the fragment stage only forwards the lit color.

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = vColor;
}
