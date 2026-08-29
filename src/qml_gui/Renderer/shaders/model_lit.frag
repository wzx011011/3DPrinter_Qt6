#version 440

// v5.15 (MODELLIT): passthrough for the per-vertex lit color (upstream
// gouraud pattern); P15.3 (OUTOFBED) adds the upstream print-volume
// boundary darkening of gouraud.fs:154-171.
//
// PrintVolumeDetection struct mirrors upstream gouraud.fs:11-22 exactly.
// Uniform data is expressed in the Qt scene frame (X=right, Y=up/height,
// Z=toward-viewer) which is the frame of the CPU-baked world positions;
// upstream uses Z=up. The axis remap applied below is (X, Z, Y)_qt.
// Upstream gates the check per volume via `partly_inside`
// (3DScene.cpp:967-976: only partly-inside volumes receive the uniform,
// others get type -1); the QRhi path keeps one shared uniform block, so the
// same per-fragment test runs against every lit fragment. Fragments of
// fully-inside volumes always pass the test unchanged, so the only visual
// difference is that fully-outside volumes darken too (upstream leaves them
// to the red background signal).

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 vWorldPos;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;                 // offset 0
  vec4 gizmoCenter;         // offset 64
  mat4 view;                // offset 80
  float printVolumeType;    // offset 144: 0 = rectangle, 1 = circle, <0 = disabled
  vec4 printVolumeXyData;   // offset 160: rect (minX, minZ, maxX, maxZ) / circle (cx, cz, radius, 0)
  vec2 printVolumeZData;    // offset 176: (zMin, zMax) on the Qt Y (height) axis
};

layout(location = 0) out vec4 fragColor;

const vec3 ZERO = vec3(0.0, 0.0, 0.0);

void main()
{
  vec4 color = vColor;

  // if the fragment is outside the print volume -> use darker color
  // (gouraud.fs:154-168, remapped to the Qt scene axes).
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
  color.rgb = (any(lessThan(pv_check_min, ZERO)) || any(greaterThan(pv_check_max, ZERO)))
      ? mix(color.rgb, ZERO, 0.3333)
      : color.rgb;

  fragColor = color;
}
