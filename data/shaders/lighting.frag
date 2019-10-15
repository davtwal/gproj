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
  vec3 atten;
  float radius;
  int type;
};

#define MAX_DYNAMIC_LIGHTS 128
#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2

layout(binding = 4) uniform DynamicLightUBO {
  Light at[MAX_DYNAMIC_LIGHTS];
} dynLights;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 sampledPos = texture(inGBuffPosition, inUV);
    vec4 sampledColor = texture(inGBuffColor, inUV);
    vec4 sampledNormal = texture(inGBuffNormal, inUV);
    
    float inSpecExp = sampledColor.w;
    vec3 inPos = sampledPos.xyz;
    vec3 inColor = sampledColor.xyz;
    
    vec3 N = normalize(sampledNormal.xyz);
    vec3 V = normalize(cam.eye - inPos);
    
    vec3 color = vec3(0, 0, 0);
    for(int i = 0; i < MAX_DYNAMIC_LIGHTS; ++i) {
      vec3 addedColor = vec3(0, 0, 0);
      vec3 toPoint = dynLights.at[i].pos - inPos;
      float dist = length(toPoint);
      
      if(dist > dynLights.at[i].radius)
        continue;
      
      vec3 L = normalize(toPoint);
      
      float n_l = max(dot(N, L), 0);
      
      addedColor += inColor * n_l * dynLights.at[i].color;
      
      // specular
      vec3 R = reflect(-L, N);
      float r_v = pow(max(dot(R, V), 0), inSpecExp);
      
      addedColor += inColor * r_v * dynLights.at[i].color;
      
      // attenuation
      vec3 atten_func = dynLights.at[i].atten;
      float attenuation_factor = 
        atten_func.x + atten_func.y * dist + atten_func.z * dist * dist;
      
      if(attenuation_factor > 0)
        addedColor *= 1 / attenuation_factor;
      
      color += addedColor;
    }
    fragColor = vec4(color.xyz, 1);
}