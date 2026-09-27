#version 440

// SHADER-PORT v6: upstream truth 140/gouraud_light.fs:11-14 (no clamp; the
// achromatic specular adds on top of material * intensity and lets the
// framebuffer saturate) + 140/gouraud.fs:250-263 print-volume boundary test +
// 140/hotbed.fs:43 bed-model variant. Like upstream, the print-volume mix is
// applied to the RAW material color BEFORE the intensity composition
// (gouraud.fs:263 then :289, hotbed.fs:43 then :45).
//
// PrintVolumeDetection struct mirrors upstream gouraud.fs:11-22 (as float
// ranges; type < 0 = ungated volume -- upstream injects type=-1 for volumes
// without detection, 3DScene.cpp:260).
// Uniform data is expressed in the Qt scene frame (X=right, Y=up/height,
// Z=toward-viewer) which is the frame of the CPU-baked world positions;
// upstream uses Z=up. The axis remap applied below is (X, Z, Y)_qt.
// Upstream gates the check per volume via `partly_inside`
// (3DScene.cpp:967-976: only partly-inside volumes receive the uniform,
// others get type -1); the QRhi path keeps one shared uniform block for the
// model volumes, so the same per-fragment test runs against every lit model
// fragment. Fragments of fully-inside volumes always pass the test
// unchanged, so the only visual difference is that fully-outside volumes
// darken too (upstream leaves them to the red background signal).
//
// mixTarget selects the outside mix target color:
//   0.0 -> ZERO  (model volumes, gouraud.fs:263)
//   1.0 -> WHITE (bed frame model, hotbed.fs:43; upstream draws the frame
//          with the hotbed shader whose outside mix lifts toward WHITE --
//          the "light outer rim", not a dark edge)

layout(location = 0) in vec4 vColor;   // baked uniform_color (upstream render_color)
layout(location = 1) in vec2 vIntensity;
layout(location = 2) in vec3 vWorldPos;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;                 // offset 0
  vec4 gizmoCenter;         // offset 64
  mat4 view;                // offset 80
  float printVolumeType;    // offset 144: 0 = rectangle, 1 = circle, <0 = ungated
  vec4 printVolumeXyData;   // offset 160: rect (minX, minZ, maxX, maxZ) / circle (cx, cz, radius, 0)
  vec2 printVolumeZData;    // offset 176: (zMin, zMax) on the Qt Y (height) axis
  float emissionFactor;     // offset 184: gouraud_light.fs:4 additive term; prepare model pass = 0.0
  float mixTarget;          // offset 188: 0.0 = outside -> ZERO, 1.0 = outside -> WHITE
};

layout(location = 0) out vec4 fragColor;

const vec3 ZERO = vec3(0.0, 0.0, 0.0);
const vec3 WHITE = vec3(1.0, 1.0, 1.0);

void main()
{
  vec4 color = vColor;

  // if the fragment is outside the print volume -> mix toward the target
  // (gouraud.fs:250-263 / hotbed.fs:34-43, remapped to the Qt scene axes)
  vec3 pv_check_min = ZERO;
  vec3 pv_check_max = ZERO;
  if (printVolumeType > -0.5 && printVolumeType < 0.5) {
    // rectangle: horizontal extent on Qt X/Z, height extent on Qt Y
    pv_check_min = vWorldPos.xyz - vec3(printVolumeXyData.x, printVolumeZData.x, printVolumeXyData.y);
    pv_check_max = vWorldPos.xyz - vec3(printVolumeXyData.z, printVolumeZData.y, printVolumeXyData.w);
  } else if (printVolumeType > 0.5) {
    // circle: radius test on the Qt XZ plane (upstream tests world_pos.xy)
    float delta_radius = printVolumeXyData.z - distance(vWorldPos.xz, printVolumeXyData.xy);
    pv_check_min = vec3(delta_radius, vWorldPos.y - printVolumeZData.x, 0.0);
    pv_check_max = vec3(0.0, vWorldPos.y - printVolumeZData.y, 0.0);
  }
  const vec3 pvMixTarget = (mixTarget > 0.5) ? WHITE : ZERO;
  color.rgb = (any(lessThan(pv_check_min, ZERO)) || any(greaterThan(pv_check_max, ZERO)))
      ? mix(color.rgb, pvMixTarget, 0.3333)
      : color.rgb;

  // gouraud_light.fs:13 -- no clamp; highlight is achromatic white
  fragColor = vec4(vec3(vIntensity.y) + color.rgb * (vIntensity.x + emissionFactor), color.a);
}
