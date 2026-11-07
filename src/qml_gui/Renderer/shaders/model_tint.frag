#version 440

// O1 (TINT): flat per-batch uniform color. No lighting -- the pick
// thumbnail and the ghost shell are unlit flat fills (the pick pass reads
// the palette color back; the ghost blends the extruder tint at the
// upstream 0.5 alpha, GCodeViewer.cpp:2330-2337).

layout(location = 0) in vec4 vColor;    // unused here
layout(location = 1) in vec3 vWorldPos; // unused here
layout(location = 2) in vec4 vTint;

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = vTint;
}
