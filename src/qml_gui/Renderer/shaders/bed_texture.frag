#version 440

// v5.15 (BEDTEX): samples the printer bed_texture image (PNG/SVG raster).
// Upstream renders it blended over the plate background/grid with depth
// writes off (PartPlate::render_logo_texture: GL_BLEND + DepthMask(FALSE)).

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D bedTexture;

// Manual anisotropic sampling. Upstream relies on
// GL_TEXTURE_MAX_ANISOTROPY_EXT set to the GPU maximum
// (GLTexture.cpp:189-193, OpenGLManager.cpp:120-122); QRhi (Qt 6.4) exposes
// no anisotropy knob, so without this the trilinear sampler picks the LOD of
// the most-compressed axis and the bed visibly sharpens/blurs while the
// camera orbits. Reproduces the upstream effect backend-independently:
// walk the minor-axis trilinear LOD and average taps along the major axis.
const int kMaxAnisoTaps = 16;

vec4 anisoSample(vec2 uv, vec2 du, vec2 dv)
{
  vec2 texSize = vec2(textureSize(bedTexture, 0));
  float footU = length(du * texSize);
  float footV = length(dv * texSize);
  // Major axis = the direction the footprint stretches along.
  vec2 major = footU >= footV ? du : dv;
  float minorFoot = min(footU, footV);
  float majorFoot = max(footU, footV);
  int taps = int(clamp(ceil(majorFoot / max(minorFoot, 1e-6)), 1.0, float(kMaxAnisoTaps)));
  // Each tap covers majorFoot/taps along the major axis. When the stretch
  // ratio exceeds the tap budget that exceeds minorFoot, so the LOD must rise
  // to match — otherwise high-frequency content aligned with the major axis
  // (the printed grid on the bed photo) aliases BETWEEN taps and strobes
  // while the camera orbits, even though the minor axis is properly averaged.
  float lod = log2(max(minorFoot, majorFoot / float(taps)));
  vec4 sum = vec4(0.0);
  for (int i = 0; i < taps; ++i) {
    float t = (float(i) + 0.5) / float(taps) - 0.5;
    sum += textureLod(bedTexture, uv + major * t, lod);
  }
  return sum / float(taps);
}

void main()
{
  fragColor = anisoSample(vUv, dFdx(vUv), dFdy(vUv));
}
