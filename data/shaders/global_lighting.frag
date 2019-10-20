#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "lighting.glsl"

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
} cam;

layout(binding = 1) uniform ShadowLights {
  ShadowLight at[MAX_GLOBAL_LIGHTS];
} lights;

layout(binding = 2) uniform sampler2D inGBuffPosition;
layout(binding = 3) uniform sampler2D inGBuffNormal;
layout(binding = 4) uniform sampler2D inGBuffColor;
layout(binding = 5) uniform sampler2D shadowMap[MAX_GLOBAL_LIGHTS];

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 sampledPos = texture(inGBuffPosition, inUV);
  vec4 sampledColor = texture(inGBuffColor, inUV);
  vec4 sampledNormal = texture(inGBuffNormal, inUV);
  
  if(sampledNormal == vec4(0, 0, 0, 0)) {
    fragColor = vec4(0, 0, 0, 0);
    return;
  }
  
  float inSpecExp = sampledColor.w;
  vec3 inPos = sampledPos.xyz;
  vec3 inColor = sampledColor.xyz;
  
  vec3 N = normalize(sampledNormal.xyz);
  vec3 V = normalize(cam.eye - inPos);
  
  mat4 shadowBias = mat4( .5,  0,  0, 0,
                           0, .5,  0, 0,
                           0,  0,  1, 0,
                          .5, .5,  0, 1);
  
  vec3 color = vec3(0, 0, 0);
  for(int i = 0; i < MAX_GLOBAL_LIGHTS; ++i) {
    vec4 shadowCoord =  shadowBias * lights.at[i].proj *  lights.at[i].view * vec4(inPos, 1.f);
     
    vec2 shadowIndex = shadowCoord.xy / shadowCoord.w;
    float pixelDepth = shadowCoord.z;
    
    if(shadowCoord.w > 0 && shadowIndex.x >= 0 && shadowIndex.y >= 0 && shadowIndex.x <= 1 && shadowIndex.y <= 1) {
      vec4 lightDepth = texture(shadowMap[i], shadowIndex);
      if(pixelDepth - lightDepth.r <= 0.1) {
        color = ComputeShadowLighting(lights.at[i], inColor, inPos, N, V, inSpecExp);
      }
    }
  }
  
  fragColor = vec4(color, 1);
}