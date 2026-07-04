#version 440

// Phase 68: gizmo fragment shader. Pure color passthrough (the per-vertex
// color from GizmoGeometry::kAxisColorRGB already carries the axis RGBA).
layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = vColor;
}
