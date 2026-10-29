#version 440

// P15.8 (BOLDGRID) screen-space rewrite. Upstream render_grid draws the
// bolder every-5th-line set through the dashed_thick_lines geometry shader
// (PartPlate.cpp:1039-1052, resources/shaders/140/dashed_thick_lines.gs):
// each line expands to a constant-width screen-space strip, so the line is
// the same pixel width at any zoom/orbit. The QRhi port originally baked a
// 0.5mm world-space quad, whose pixel width swung with the camera (the
// shimmering grid report). QRhi exposes no geometry stage, so the same
// expansion lives here: each vertex carries BOTH line endpoints plus its
// (end, side) corner; the vertex shader builds the pixel-space perpendicular
// and pushes its endpoint out by the quad half width.

layout(location = 0) in vec3 p0;
layout(location = 1) in vec3 p1;
layout(location = 2) in vec2 corner;  // x = endpoint (0/1), y = side (-1/+1)
layout(location = 3) in vec4 color;
layout(location = 4) in float spacingMm;  // this set's grid pitch (mm)

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;
};

layout(std140, binding = 1) uniform BoldLineParams
{
  vec4 params;  // xy = viewport size (px), z = core width (px), w = aa radius (px)
};

layout(location = 0) out vec4 vColor;
layout(location = 1) out float vSidePx;

out gl_PerVertex
{
  vec4 gl_Position;
};

// NDC spans [-1, 1] = 2.0 across the viewport, so one NDC unit is
// viewport/2 pixels. Every NDC<->pixel conversion below must go through
// this; multiplying by the raw viewport size (the old bug) inflated every
// pixel quantity by 2x and shifted the density fade window half an octave
// away from its designed thresholds (fine grid stayed fully opaque inside
// the strobing zone while orbiting).
vec2 pxPerNdc()
{
  return params.xy * 0.5;
}

void main()
{
  vec4 clip0 = mvp * vec4(p0, 1.0);
  vec4 clip1 = mvp * vec4(p1, 1.0);
  vec2 ndc0 = clip0.xy / clip0.w;
  vec2 ndc1 = clip1.xy / clip1.w;
  const vec2 pxScale = pxPerNdc();
  // Perpendicular in pixel space, then back to NDC for the position offset.
  vec2 pxSpan = (ndc1 - ndc0) * pxScale;
  float pxLen = length(pxSpan);
  vec2 pxNormal = pxLen > 1e-5 ? vec2(-pxSpan.y, pxSpan.x) / pxLen : vec2(0.0, 1.0);
  // Upstream: line_width = max(1.0, width) + 2 * aa_radius (dashed_thick_lines.gs).
  float lineWidth = max(params.z, 1.0) + 2.0 * params.w;
  float quadHalf = 0.5 * lineWidth;
  vec2 offsetPx = pxNormal * (corner.y * quadHalf);

  bool firstEnd = corner.x < 0.5;
  vec2 ndc = firstEnd ? ndc0 : ndc1;
  vec4 clip = firstEnd ? clip0 : clip1;
  gl_Position = vec4(ndc + offsetPx / pxScale, clip.z, clip.w) * vec4(clip.w, clip.w, 1.0, 1.0);
  // Pixel distance from the line center at this quad edge; the FS interpolates it.
  vSidePx = corner.y * quadHalf;

  // Grid-density fade, evaluated AT THIS VERTEX'S ENDPOINT. Perspective
  // compresses the far half of a long line much harder than the near half,
  // so a single midpoint value left the far end undersampled while the near
  // end was already fading; per-end evaluation gives a smooth along-line
  // gradient through the interpolation. Once adjacent lines land closer
  // than the pixel budget the strip set undersamples and strobes (the
  // grazing-angle flicker), so fade it out. The bolder set's 5x pitch fades
  // proportionally later, preserving the hierarchy to shallower angles.
  float fade = 1.0;
  vec3 lineDirW = p1 - p0;
  vec3 perpW = cross(lineDirW, vec3(0.0, 1.0, 0.0));  // bed plane normal is +Y
  float perpLen = length(perpW);
  vec3 endW = firstEnd ? p0 : p1;
  vec4 endClip = firstEnd ? clip0 : clip1;
  if (perpLen > 1e-5 && endClip.w > 1e-4) {
    perpW /= perpLen;
    vec4 stepClip = mvp * vec4(endW + spacingMm * perpW, 1.0);
    vec2 endNdc = endClip.xy / endClip.w;
    vec2 stepNdc = stepClip.xy / max(stepClip.w, 1e-6);
    float spacingPx = length((stepNdc - endNdc) * pxScale);
    // Full alpha at >= 8.5px pitch, gone at <= 2.5px — fade early enough
    // that the approaching-undersample zone never becomes visible motion.
    fade = clamp((spacingPx - 2.5) / 6.0, 0.0, 1.0);
  }
  vColor = vec4(color.rgb, color.a * fade);
}
