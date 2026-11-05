#version 440

// FIX5M (P3 derivative lighting): the per-face normal is reconstructed from
// screen-space derivatives of the world position -- flat shading identical to
// the CPU per-face normal bake, with no normal attribute and no per-upload
// normal recompute. The gouraud_light equations are the model_lit pair
// (upstream truth 140/gouraud_light.fs:11-14 + gouraud.fs:250-263), moved to
// fragment stage because the normal only exists per-fragment here.

layout(location = 0) in vec4 vColor;   // baked uniform_color (upstream render_color)
layout(location = 1) in vec3 vWorldPos;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;                 // offset 0
  vec4 gizmoCenter;         // offset 64
  mat4 view;                // offset 80
  float printVolumeType;    // offset 144: 0 = rectangle, 1 = circle, <0 = ungated
  vec4 printVolumeXyData;   // offset 160
  vec2 printVolumeZData;    // offset 176
  float emissionFactor;     // offset 184
  float mixTarget;          // offset 188
};

layout(location = 0) out vec4 fragColor;

const vec3 ZERO = vec3(0.0, 0.0, 0.0);
const vec3 WHITE = vec3(1.0, 1.0, 1.0);

const vec3 LIGHT_TOP_DIR = vec3(-0.4574957, 0.4574957, 0.7624929); // eye space, gouraud_light.vs:6
const vec3 LIGHT_FRONT_DIR = vec3(0.6985074, 0.1397015, 0.6985074); // eye space, gouraud_light.vs:12
const float INTENSITY_AMBIENT = 0.3;    // gouraud_light.vs:15
const float LIGHT_TOP_DIFFUSE = 0.48;   // 0.8 * INTENSITY_CORRECTION
const float LIGHT_TOP_SPECULAR = 0.075; // 0.125 * INTENSITY_CORRECTION
const float LIGHT_TOP_SHININESS = 20.0; // gouraud_light.vs:9
const float LIGHT_FRONT_DIFFUSE = 0.18; // 0.3 * INTENSITY_CORRECTION

void main()
{
  vec4 color = vColor;

  // FIX5M: flat per-face normal from screen-space derivatives. dFdx/dFdy of
  // the world position span the triangle's plane in pixel space; the cross
  // product is the face normal (magnitude cancels in normalize).
  const vec3 flatNormal = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
  const vec3 eyeNormal = normalize(mat3(view) * flatNormal);
  const vec4 eyePos = view * vec4(vWorldPos, 1.0);

  // gouraud_light.vs:33-45, two-sided (see model_p3.vert note)
  float intensity = INTENSITY_AMBIENT;
  float NdotL = max(dot(eyeNormal, LIGHT_TOP_DIR), dot(-eyeNormal, LIGHT_TOP_DIR));
  intensity += NdotL * LIGHT_TOP_DIFFUSE;
  const float specular = LIGHT_TOP_SPECULAR
      * pow(max(dot(-normalize(eyePos.xyz), reflect(-LIGHT_TOP_DIR, eyeNormal)), 0.0),
            LIGHT_TOP_SHININESS);

  NdotL = max(dot(eyeNormal, LIGHT_FRONT_DIR), dot(-eyeNormal, LIGHT_FRONT_DIR));
  intensity += NdotL * LIGHT_FRONT_DIFFUSE;

  // print-volume boundary test (gouraud.fs:250-263 / hotbed.fs:34-43)
  vec3 pv_check_min = ZERO;
  vec3 pv_check_max = ZERO;
  if (printVolumeType > -0.5 && printVolumeType < 0.5) {
    pv_check_min = vWorldPos - vec3(printVolumeXyData.x, printVolumeZData.x, printVolumeXyData.y);
    pv_check_max = vWorldPos - vec3(printVolumeXyData.z, printVolumeZData.y, printVolumeXyData.w);
  } else if (printVolumeType > 0.5) {
    float delta_radius = printVolumeXyData.z - distance(vWorldPos.xz, printVolumeXyData.xy);
    pv_check_min = vec3(delta_radius, vWorldPos.y - printVolumeZData.x, 0.0);
    pv_check_max = vec3(0.0, vWorldPos.y - printVolumeZData.y, 0.0);
  }
  const vec3 pvMixTarget = (mixTarget > 0.5) ? WHITE : ZERO;
  color.rgb = (any(lessThan(pv_check_min, ZERO)) || any(greaterThan(pv_check_max, ZERO)))
      ? mix(color.rgb, pvMixTarget, 0.3333)
      : color.rgb;

  // gouraud_light.fs:13 -- no clamp; achromatic highlight adds on top
  fragColor = vec4(vec3(specular) + color.rgb * (intensity + emissionFactor), color.a);
}
