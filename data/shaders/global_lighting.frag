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

vec3 CholeskyDecomp(vec4 bv, float z) {
  mat3 m = mat3(   1, bv.x, bv.y,
                bv.x, bv.y, bv.z,
                bv.y, bv.z, bv.w);
  
  float a = sqrt(m[0][0]);
  float b = m[0][1] / a;
  float c = m[0][2] / a;
  float d = sqrt(m[1][1] - b * b);
  float e = (m[1][2] - b * c) / d;
  float f = sqrt(m[2][2] - c * c - e * e);
  
  vec3 ch;
  ch[0] = 1 / a;
  ch[1] = (z - b * ch[0]) / d;
  ch[2] = (z * z - c * ch[0] - e * e);
  
  vec3 cv;
  cv[2] =  ch[2] / f;
  cv[1] = (ch[1] - e * cv[2]) / d;
  cv[0] = (ch[0] - b * cv[1] - c * cv[2]) / a;
  
  return cv;
}

float getG(vec4 moments, float fragDepth) {
  float bias = 0.00003;
  float zf = fragDepth;
  vec4 b = moments * (1 - bias) + bias * vec4(0.5, 0.5, 0.5, 0.5);
  
  vec3 c = CholeskyDecomp(b, zf);
  
  float disc = c[1] * c[1] - 4 * c[2] * c[0];
  
  disc = sqrt(disc);
  
  float z2 = (-c[1] - disc) / (2 * c[2]);
  float z3 = (-c[1] + disc) / (2 * c[2]);
  
  float G = 0;
  
  // /// /// Final Formulation of G
  if(zf < z2) {    
    G = 0;
  }
  else if(zf < z3) {    
    float numer = zf * z3 - b.x * (zf + z3) + b.y;
    float denom = (z3 - z2) * (zf - z2);
    
    G = numer / denom;
  } 
  else {
    float numer = z2 * z3 - b.x * (z2 + z3) + b.y;
    float denom = (zf - z2) * (zf - z3);
    
    G = 1 - (numer / denom);
  }
  
  // Check to make sure c is a valid solution
  //mat3 bmat = mat3(1, b.x, b.y, b.x, b.y, b.z, b.y, b.z, b.w);
  //vec3 mult = bmat * c;
  
  //if(abs(mult.x - 1) > 0.001)
  //  return 2.0;
  //
  //if(abs(mult.y - zf) > 0.001)
  //  return 3.0;
  
  // NOTE: this is sometimes true, mainly if in true shadow.
  //if(abs(mult.z - zf * zf) > 0.001)
  //  return 4.0;
  
  // /// /// Quadratic Formula Step
  // now: c0 + z * c1 + z^2 * c2 = 0
  // c[2] = a, c[1] = b, c[0] = c
  
  //if(disc < 0)
  //  return 5.0; // error
  
  //if(abs(c[2]) < 0.001)
  //  return 8.0;
  
  //if(isnan(z2) || isinf(z2))
  //  return 7.0;
  //
  //if(isnan(z3) || isinf(z3))
  //  return 7.0;
  return G;
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
    
    if(shadowCoord.w > 0 && shadowIndex.x >= 0 && shadowIndex.y >= 0 && shadowIndex.x <= 1 && shadowIndex.y <= 1) {
      vec4 lightDepth = texture(shadowMap[i], shadowIndex);
      float pixelDepth = shadowCoord.z;
      pixelDepth = (pixelDepth - lights.at[i].nearDist) / (lights.at[i].farDist - lights.at[i].nearDist);
      
      float G = getG(lightDepth, pixelDepth);
      
      //switch(int(G)) {
      //case 2: // (B * c).x != 1 ; c is not a valid solution
      //  color += vec3(0, 0, 1); // blue
      //  break;
      //  
      //case 3: // (B * c).y != zf ; c is not a valid solution
      //  color += vec3(1, 0, 0); // red
      //  break;
      //  
      //case 4: // (B * c).z != zf^2 ; c is not a valid solution
      //  color += vec3(1, 0, 1); // purple
      //  break;
      //  
      //case 5: // discriminant was negative
      //  color += vec3(0, 1, 1); // cyan
      //  break;
      //
      //case 6: // a denominator while computing G was 0
      //  color += vec3(1, 1, 1); // white
      //  break;
      //  
      //case 7: // a number was nan or inf
      //  color += vec3(1, 1, 0); // yellow
      //  break;
      //  
      //case 8: // c[2] was 0
      //  color += vec3(0, 1, 0);
      //  break;
      //}
      if(G < 1)
        color += (1 - G) * ComputeShadowLighting(lights.at[i], inColor, inPos, N, V, inSpecExp);     
    }
  }
  
  fragColor = vec4(color, 1);
}