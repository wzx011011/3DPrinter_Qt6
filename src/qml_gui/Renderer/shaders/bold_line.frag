#version 440

// P15.8 (BOLDGRID): analytic edge feather for the screen-space bolder grid
// lines. Deliberately declares NO uniform block: sharing BoldLineParams with
// the vertex stage made the GL link fail with a cross-stage struct mismatch
// (spirv-cross lowering quirk), and the FS only needs the core/aa constants
// — the same 1px core + 0.5px radius the VS reads from the params UBO
// (upstream render_grid width uniform clamps to the 1px core,
// dashed_thick_lines.gs aa_radius 0.5).

layout(location = 0) in vec4 vColor;
layout(location = 1) in float vSidePx;
layout(location = 0) out vec4 fragColor;

void main()
{
  // Full alpha inside the half-core, falling to zero across the aa radius at
  // the quad edge. (The literal upstream normalized form degenerates at these
  // values — its feather window lands entirely outside the quad, leaving
  // hard 2px edges that strobe while the camera moves.)
  float halfCore = 0.5;
  float alpha = 1.0 - smoothstep(halfCore, halfCore + 0.5, abs(vSidePx));
  fragColor = vec4(vColor.rgb, vColor.a * alpha);
}
