#version 440

// SHADER-PORT v6: upstream truth third_party/OrcaSlicer/resources/shaders/
// 140/gouraud_light.vs:3-45 (equation identical to 140/gouraud.vs:56-71 and
// 140/hotbed.vs:28-46; verified by diff). INTENSITY_CORRECTION 0.6 is folded
// into the diffuse/specular constants below, exactly as the upstream #defines
// evaluate (0.8*0.6, 0.125*0.6, 0.3*0.6).
// The two light directions are EYE-SPACE constants: there is no world-space
// light, so the light follows the camera (gouraud_light.vs:6/:12; C++ shadow
// light uses the same direction, GLCanvas3D.cpp:8008).
// The world-space position is forwarded to the fragment stage for the
// upstream print-volume boundary test (gouraud.fs:250-263). The CPU bake in
// ProjectServiceMock::meshData stores world coordinates in the vertex buffer,
// so `position` IS the upstream world_pos (gouraud.vs:72 computes world_pos =
// volume_world_matrix * position; here the matrix is already applied on the
// CPU -- no second Z-up/Y-up transform here, that swap happened at upload).
// P15.3 -> v6 split: the lit color is no longer composed per-vertex. The raw
// baked uniform_color (loc 0) and the vec2 intensity (loc 1) go to the
// fragment stage, which composes them per gouraud_light.fs:13 (no clamp, the
// achromatic highlight adds on top and lets the framebuffer saturate).
// Tracked deviation (port spec 1.3-3/1.5): upstream gouraud_light is
// single-sided (gouraud_light.vs:34), but the CPU bake applies a pure Y/Z
// axis swap (a reflection) to every vertex and only rewinds mirrored batches
// (ProjectServiceMock.cpp:12415-12417), so the recomputed per-face normals
// cannot guarantee upstream-consistent outward orientation. The two-sided
// max(+dot, -dot) is kept; note it is invariant under a global normal flip
// (and reflect() is too), so the deviation is limited to faces whose true
// outward normal faces away from a light (dark side receives the front light).

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;         // offset 0
  vec4 gizmoCenter; // offset 64 (.xyz center, .w scale) -- unused here
  mat4 view;        // offset 80
};

layout(location = 0) out vec4 vColor;    // raw baked uniform_color (composition in frag)
layout(location = 1) out vec2 vIntensity; // x = diffuse (ambient + NdotL terms), y = specular
layout(location = 2) out vec3 vWorldPos;

const vec3 LIGHT_TOP_DIR = vec3(-0.4574957, 0.4574957, 0.7624929); // eye space, gouraud_light.vs:6
const vec3 LIGHT_FRONT_DIR = vec3(0.6985074, 0.1397015, 0.6985074); // eye space, gouraud_light.vs:12
const float INTENSITY_AMBIENT = 0.3;    // gouraud_light.vs:15
const float LIGHT_TOP_DIFFUSE = 0.48;   // 0.8 * INTENSITY_CORRECTION, gouraud_light.vs:7
const float LIGHT_TOP_SPECULAR = 0.075; // 0.125 * INTENSITY_CORRECTION, gouraud_light.vs:8
const float LIGHT_TOP_SHININESS = 20.0; // gouraud_light.vs:9
const float LIGHT_FRONT_DIFFUSE = 0.18; // 0.3 * INTENSITY_CORRECTION, gouraud_light.vs:13

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  gl_Position = mvp * vec4(position, 1.0);
  vColor = color;
  vWorldPos = position;

  const vec3 eyeNormal = normalize(mat3(view) * normal);
  const vec4 eyePos = view * vec4(position, 1.0);

  float NdotL = max(dot(eyeNormal, LIGHT_TOP_DIR), dot(-eyeNormal, LIGHT_TOP_DIR));
  vIntensity.x = INTENSITY_AMBIENT + NdotL * LIGHT_TOP_DIFFUSE;
  vIntensity.y = LIGHT_TOP_SPECULAR
      * pow(max(dot(-normalize(eyePos.xyz), reflect(-LIGHT_TOP_DIR, eyeNormal)), 0.0),
            LIGHT_TOP_SHININESS);

  NdotL = max(dot(eyeNormal, LIGHT_FRONT_DIR), dot(-eyeNormal, LIGHT_FRONT_DIR));
  vIntensity.x += NdotL * LIGHT_FRONT_DIFFUSE;
}
