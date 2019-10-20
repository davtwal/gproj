#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "lighting.glsl"

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
} cam;

layout(binding = 1) uniform sampler2D inGBuffPosition;
layout(binding = 2) uniform sampler2D inGBuffNormal;
layout(binding = 3) uniform sampler2D inGBuffColor;

layout(binding = 4) uniform sampler2D previousImage;

layout(binding = 5) uniform DynamicLightUBO {
  Light at[MAX_DYNAMIC_LOCAL_LIGHTS];
} dynLights;


layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

void main() {
  vec4 sampledPos = texture(inGBuffPosition, inUV);
  vec4 sampledColor = texture(inGBuffColor, inUV);
  vec4 sampledNormal = texture(inGBuffNormal, inUV);
  vec4 previousColor = texture(previousImage, inUV);
  
  float inSpecExp = sampledColor.w;
  vec3 inPos = sampledPos.xyz;
  vec3 inColor = sampledColor.xyz;
  
  vec3 N = normalize(sampledNormal.xyz);
  vec3 V = normalize(cam.eye - inPos);
  
  vec3 color = vec3(0, 0, 0);
  for(int i = 0; i < MAX_DYNAMIC_LOCAL_LIGHTS; ++i) {
    color += ComputeLighting(dynLights.at[i], inColor, inPos, N, V, inSpecExp);
  }
  
  fragColor = vec4(color.xyz, 1) + previousColor;
  //fragColor /= fragColor.a;
}