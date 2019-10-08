#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
} cam;

layout(binding = 1) uniform sampler2D inGBuffPosition;
layout(binding = 2) uniform sampler2D inGBuffNormal;
layout(binding = 3) uniform sampler2D inGBuffColor;

struct Light {
  vec3 pos;
  vec3 dir;
  vec3 color;
  float radius;
  int type;
};

#define MAX_DYNAMIC_LIGHTS 2
#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2

layout(binding = 4) uniform DynamicLightUBO {
  Light at[MAX_DYNAMIC_LIGHTS];
} dynLights;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 inPos = texture(inGBuffPosition, inUV).xyz;
    vec3 inColor = texture(inGBuffColor, inUV).xyz;
    
    vec3 N = normalize(texture(inGBuffNormal, inUV).xyz);
    vec3 V = normalize(cam.eye - inPos);
    
    vec3 color = vec3(0, 0, 0);
    for(int i = 0; i < MAX_DYNAMIC_LIGHTS; ++i) {
      // diffuse only for the moment
      // as objects/materials do not have anything other than diffuse color atm
      vec3 L = normalize(dynLights.at[i].pos - inPos);
      float n_l = max(dot(N, L), 0);
      
      color += inColor * n_l;
      
      // specular
      //vec3 R = reflect(-L, N);
      //float r_v = pow(max(dot(R, V), 0), 100);
      //
      //color += inColor * r_v;
    }
    fragColor = vec4(color, 1);
}