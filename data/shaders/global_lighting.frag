#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "lighting.glsl"

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
} lights;

layout(binding = 2) uniform sampler2D inGBuffPosition;
layout(binding = 3) uniform sampler2D inGBuffNormal;
layout(binding = 4) uniform sampler2D inGBuffColor;
layout(binding = 5) uniform sampler2D shadowMap[MAX_GLOBAL_LIGHTS];

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;
float det3(vec3 a, vec3 b, vec3 c) 
{
  return a.x*(b.y*c.z-b.z*c.y) + a.y*(b.z*c.x-b.x*c.z) + a.z*(b.x*c.y-b.y*c.x); 
}

vec3 CramersRule(vec4 b, float z) {
  vec3 A = vec3(1, b.x, b.y);
  vec3 B = b.xyz;
  vec3 C = b.yzw;
  vec3 Z = vec3(1, z, z *z);
  
  float det = det3(A,B,C);
  float d = 1 / det3(A,B,C);
  return vec3(
    det3(Z,B,C) * d,
    det3(A,Z,C) * d,
    det3(A,B,Z) * d
  );
}

//vec3 CholeskyDecomp(vec4 b, float z) {
//  
//}

float getG(vec4 moments, float fragDepth, float halfDist) {
  float bias = 0.00003;
  float depthBias = 0;
  vec3 z = vec3(0.0);
  z[0] = fragDepth - depthBias;
  vec4 b = moments * (1 - bias) + bias * vec4(halfDist, halfDist, halfDist, halfDist);
  
  vec3 c = CramersRule(b, z[0]);
  
  mat3 bmat = mat3(1, b.x, b.y, b.x, b.y, b.z, b.y, b.z, b.w);
  
  vec3 A = vec3(1, b.x, b.y);
  vec3 B = b.xyz;
  vec3 C = b.yzw;
  vec3 Z = vec3(1, z.x, z.x *z.x);
  
  float det = det3(A,B,C);
  //if(abs(det) < 0.0001)
  //  return 1.0;
  
  //if(isnan(c.x) || isinf(c.x))
  //  return 1.0;
  //
  vec3 mult = bmat * c;
  
  //if(abs(mult.x - 1) < 0.01)
  //  return 2.0;
  
  //if(abs(mult.y - z[0]) < 0.01)
  //  return 3.0;
  
  //if(abs(mult.z - z[0] * z[0]) < 0.01)
  //  return 4.0;
  //
  //return 0.0;
  
  // now: c0 + z * c1 + z^2 * c2 = 0
  float disc = c[1] * c[1] - 4 * c[2] * c[0];  
  float p = -c[1] / c[2];
  float q = 1 / (2 * c[2]);
  disc = sqrt(disc);
  z[1] = (p - disc) * q;
  z[2] = (p + disc) * q;
  
  if(z[0] - z[1] < 0)
    return 0;
  
  float G = 0;
  if(z[0] - z[2] < 0) {
    G = (z[0]*z[2] - b[0]*(z[0] + z[2]) + b[1]) / ((z[2]-z[1])*(z[0]-z[2]));
  } 
  else {
    G = 1 - (z[1]*z[2] - b[0]*(z[1] + z[2]) + b[1]) / ((z[0]-z[1])*(z[0]-z[2]));
  }
  
  return 1 - clamp(G, 0.0, 1.0);
}

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
    pixelDepth = (pixelDepth - lights.at[i].nearDist) / (lights.at[i].farDist - lights.at[i].nearDist);
    
    if(shadowCoord.w > 0 && shadowIndex.x >= 0 && shadowIndex.y >= 0 && shadowIndex.x <= 1 && shadowIndex.y <= 1) {
      vec4 lightDepth = texture(shadowMap[i], shadowIndex);
      float halfDist = 0.5 * (lights.at[i].nearDist + lights.at[i].farDist);
      float G = getG(lightDepth, pixelDepth, halfDist);
      
      //if(abs(lightDepth.r * lightDepth.r - lightDepth.g) > 0.001)
      //  color += vec3(1, 1, 1);
      //
      //switch(int(G)) {
      //case 0:
      //  color += vec3(1, 0, 0);
      //  break;
      //
      //case 1: // Cramers rule determinant is 0
      //  color += vec3(0, 0, 1);
      //  break;
      //
      //case 2: // [Bc].x == 1
      //  color += vec3(0, 1, 0);
      //  break;
      //  
      //case 3: // [Bc].y == zf
      //  color += vec3(0, 1, 1);
      //  break;
      //  
      //case 4: // [Bc].z == zf^2
      //  color += vec3(1, 1, 0);
      //  break;
      //  
      ////default:
      ////  color += vec3(1, 1, 1);
      //}
      
      //float G = 1;
      //if(pixelDepth - lightDepth.r < 0.01)
      
      color += (1 - G) * ComputeShadowLighting(lights.at[i], inColor, inPos, N, V, inSpecExp);
    }
  }
  
  fragColor = vec4(color, 1);
}