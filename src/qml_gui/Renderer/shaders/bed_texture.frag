#version 440

// v5.15 (BEDTEX): samples the printer bed_texture image (PNG/SVG raster).
// Upstream renders it blended over the plate background/grid with depth
// writes off (PartPlate::render_logo_texture: GL_BLEND + DepthMask(FALSE)).

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D bedTexture;

void main()
{
  fragColor = texture(bedTexture, vUv);
}
