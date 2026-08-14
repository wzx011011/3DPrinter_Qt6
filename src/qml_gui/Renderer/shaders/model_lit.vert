#version 440

// v5.15 (MODELLIT): per-vertex two-light shading for model meshes. Constants
// mirror upstream gouraud.vs exactly (third_party/OrcaSlicer/resources/
// shaders/140/gouraud.vs): ambient 0.3, top light dir/diffuse/specular/
// shininess, front light diffuse. Lighting runs in eye space; winding-
// agnostic diffuse (max of N and -N terms) because the mesh byte stream
// does not guarantee consistent triangle winding.

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;

layout(std140, binding = 0) uniform CameraBlock
{
  mat4 mvp;
  vec4 gizmoCenter;
  mat4 view;
};

layout(location = 0) out vec4 vColor;

const vec3 LIGHT_TOP_DIR = vec3(-0.4574957, 0.4574957, 0.7624929);
const vec3 LIGHT_FRONT_DIR = vec3(0.6985074, 0.1397015, 0.6985074);
const float INTENSITY_AMBIENT = 0.3;
const float LIGHT_TOP_DIFFUSE = 0.8;
const float LIGHT_TOP_SPECULAR = 0.125;
const float LIGHT_TOP_SHININESS = 20.0;
const float LIGHT_FRONT_DIFFUSE = 0.3;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  gl_Position = mvp * vec4(position, 1.0);

  const vec3 eyeNormal = normalize(mat3(view) * normal);
  const vec4 eyePos = view * vec4(position, 1.0);

  float intensity = INTENSITY_AMBIENT;
  intensity += max(dot(eyeNormal, LIGHT_TOP_DIR), dot(-eyeNormal, LIGHT_TOP_DIR)) * LIGHT_TOP_DIFFUSE;
  intensity += max(dot(eyeNormal, LIGHT_FRONT_DIR), dot(-eyeNormal, LIGHT_FRONT_DIR)) * LIGHT_FRONT_DIFFUSE;

  const float specular = LIGHT_TOP_SPECULAR
      * pow(max(dot(-normalize(eyePos.xyz), reflect(-LIGHT_TOP_DIR, eyeNormal)), 0.0), LIGHT_TOP_SHININESS);

  vColor = vec4(clamp(color.rgb * intensity + vec3(specular), 0.0, 1.0), color.a);
}
