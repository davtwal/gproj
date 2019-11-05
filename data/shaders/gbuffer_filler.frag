#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
  float farDist;
  float nearDist;
} cam;

#include "inc/defines.glsl"

//struct Material {
//  vec3 diffuseCoeff;
//  vec3 specularCoeff;
//  float metallicCoeff;  // if 0, ignore metallic sampler
//  float roughnessCoeff; // if 0, ignore roughness sampler
//  int hasNormalMap;
//  int hasMetallicMap;
//  int hasRoughnessMap;
//  //bool hasAOMap;
//};

layout(binding = 1) uniform ObjectUBO {
  mat4 model;
  //uint mtlIndex;
} obj;

//layout(binding = 2) uniform MaterialsUBO {
//  Material at[MAX_MATERIALS];
//  int count;
//} mtls;

//layout(binding = 3) uniform sampler2DArray inMtlMaps[MAX_MATERIALS];

layout(location = 0) in vec4 inWorldPosition;
layout(location = 1) in vec4 inWorldNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUV;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;

void main() {  
  vec3 pos    = inWorldPosition.xyz;
  vec3 normal = normalize(inWorldNormal.xyz);
  vec3 color  = inColor.xyz;
  float roughness = 100.f;
  float metallic = 1.f;
  
  // Each material is stored in a 3D texture with MTL_MAP_COUNT layers
  //vec3 albedoMap     = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 0));
  //
  //if(mtls.at[obj.mtlIndex].hasNormalMap == 1) {
  //  vec3 normalMap = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 1));
  //  // do normal mapping, output in 'normal'
  //}
  //
  //if(mtls.at[obj.mtlIndex].hasMetallicMap == 1) {
  //  float metallicMap  = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 2));
  //  metallic = metallicMap * mtls.at[obj.mtlIndex].metallicCoeff;
  //}
  //
  //if(mtls.at[obj.mtlIndex].hasRoughnessMap == 1) {
  //  float roughnessMap = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 3));
  //  roughness = roughnessMap * mtls.at[obj.mtlIndex].roughnessCoeff;
  //}
  //
  //color = color * albedoMap * mtls.at[obj.mtlIndex].diffuseCoeff;
  
  outPos    = vec4(inWorldPosition.xyz, 1.0);
  outNormal = vec4(normalize(inWorldNormal.xyz), metallic);
  outColor  = vec4(color, roughness);
}