#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "inc/defines.glsl"
#include "inc/lighting.glsl"

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
  float farDist;
  float nearDist;
} cam;

layout(binding = 1) uniform ShadowLights {
  ShadowLight at[MAX_GLOBAL_LIGHTS];
  uint count;
} lights;

// shader control
layout(binding = 2) SHADER_CONTROL_UNIFORM control;

layout(binding = 3) uniform sampler2D inGBuffPosition;
layout(binding = 4) uniform sampler2D inGBuffNormal;
layout(binding = 5) uniform sampler2D inGBuffColor;
layout(binding = 6) uniform sampler2D shadowMap[MAX_GLOBAL_LIGHTS];

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

float getG(vec4 moments, float fragmentDepth) {
  // Code comes from the supplementary paper for Hamburger 4MSM.
  // Converted from HLSL to GLSL.
  vec4 b = moments * (1 - control.momentBias) + control.momentBias * vec4(0.5f, 0.5f, 0.5f, 0.5f);
  vec3 z = vec3(fragmentDepth - control.depthBias, 0, 0);
  
  // Magic Cholesky decomposition
  float L32D22 = -b[0] * b[1] + b[2];
  float D22    = -b[0] * b[0] + b[1];
  float SDV    = -b[1] * b[1] + b[3];
  float D33D22 = dot( vec2(SDV, -L32D22), vec2(D22, L32D22) );
  float InvD22 = 1.0 / D22;
  float L32    = L32D22 * InvD22;
  
  vec3 c = vec3(1.0f, z[0], z[0] * z[0]);
  
  c[1] -= b.x;
  c[2] -= b.y + L32 * c[1];
  
  c[1] *= InvD22;
  c[2] *= D22 / D33D22;
  
  c[1] -= L32 * c[2];
  c[0] -= dot(c.yz, b.xy);
  
  // Quadratic formula solving
  float p = c[1] / c[2];
  float q = c[0] / c[2];
  float D = ( (p * p) / 4.0f ) - q;
  float r = sqrt(D);
  
  z[1] = -(p / 2.0f) - r;
  z[2] = -(p / 2.0f) + r;
  
  // Final calculation of G
  vec4 switchVec = (z[2] < z[0])
    ? vec4(z[1], z[0], 1.0f, 1.0f)
    : (z[1] < z[0])
      ? vec4(z[0], z[1], 0.0f, 1.0f)
      : vec4(0.f, 0.f, 0.f, 0.f);
      
  float quotient = (switchVec[0] * z[2] - b[0] * (switchVec[0] + z[2]) + b[1])
                 / ((z[2] - switchVec[1]) * (z[0] - z[1]));
                 
  return clamp(switchVec[2] + switchVec[3] * quotient, 0.0f, 1.0f);
}

void main() {
  vec4 sampledPos = texture(inGBuffPosition, inUV);
  vec4 sampledColor = texture(inGBuffColor, inUV);
  vec4 sampledNormal = texture(inGBuffNormal, inUV);
  
  float isObject    = sampledPos.w;
  float inRoughness = sampledColor.w;
  float inMetallic  = sampledNormal.w;
  vec3 inPos = sampledPos.xyz;
  vec3 inColor = sampledColor.xyz;
  
  if(int(isObject) == 1) {
    vec3 N = normalize(sampledNormal.xyz);
    vec3 V = normalize(cam.eye - inPos);
    
    mat4 shadowBias = mat4( .5,  0,  0, 0,
                            0, .5,  0, 0,
                            0,  0,  1, 0,
                            .5, .5,  0, 1);
    
    vec3 color = vec3(0, 0, 0);
    for(int i = 0; i < lights.count && control.doGlobalLighting == 1; ++i) {
      vec4 shadowCoord =  shadowBias * lights.at[i].proj *  lights.at[i].view * vec4(inPos, 1.f);
      
      vec2 shadowIndex = shadowCoord.xy / shadowCoord.w;
      
      if(shadowCoord.w > 0 && shadowIndex.x >= 0 && shadowIndex.y >= 0 && shadowIndex.x <= 1 && shadowIndex.y <= 1) {
        vec4 lightDepth = texture(shadowMap[i], shadowIndex);
        float pixelDepth = shadowCoord.z;
        pixelDepth = (pixelDepth - lights.at[i].nearDist) / (lights.at[i].farDist - lights.at[i].nearDist);
        
        float G = getG(lightDepth, pixelDepth);
        
        if(G < 1) {         
          vec3 lightColor = lights.at[i].color;
          vec3 lightPos   = lights.at[i].pos;
          vec3 lightAtten = lights.at[i].atten;
          float lightRad  = lights.at[i].radius * 100;
          
          vec3 pbrColor = computeDirectPBR(lightColor, lightPos, lightAtten, lightRad, inColor, inPos, N, V, inRoughness, inMetallic);
          
          color += (1 - G) * pbrColor;
        }
      }
    }
    
    fragColor = vec4(color, 1);
  }
  else
    fragColor = vec4(0, 0, 0, 1);
}