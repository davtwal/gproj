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

struct Material {
  vec3 diffuseCoeff;
  vec3 specularCoeff;
  float metallicCoeff;  // if 0, ignore metallic sampler
  float roughnessCoeff; // if 0, ignore roughness sampler
  int hasAlbedoMap;
  int hasNormalMap;
  int hasMetallicMap;
  int hasRoughnessMap;
  //bool hasAOMap;
};

layout(binding = 1) uniform ObjectUBO {
  mat4 model;
  uint mtlIndex;
} obj;

layout(binding = 2) uniform MaterialsUBO {
  Material at[MAX_MATERIALS];
  //int count;
} mtls;

layout(binding = 3) uniform sampler2D inMtlAlbedo[MAX_MATERIALS];
layout(binding = 4) uniform sampler2D inMtlNormal[MAX_MATERIALS];
layout(binding = 5) uniform sampler2D inMtlMetallic[MAX_MATERIALS];
layout(binding = 6) uniform sampler2D inMtlRoughness[MAX_MATERIALS];

// shader control
layout(binding = 7) uniform ShaderControl {
  float momentBias;
  float depthBias;
  float defaultRoughness;
  float defaultMetallic;
} control;

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
  vec3 pos      = inWorldPosition.xyz;
  vec3 normal   = normalize(inWorldNormal.xyz);
  vec3 tangent  = normalize(inTangent);
  vec3 bitan    = normalize(inBitangent);
  vec3 color    = inColor.xyz;
  float roughness = control.defaultRoughness;
  float metallic  = control.defaultMetallic;
  
  // Each material is stored in a 3D texture with MTL_MAP_COUNT layers
  //vec3 albedoMap     = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 0));
  if(mtls.at[obj.mtlIndex].hasAlbedoMap == 1) {
    vec3 albedoMap = pow(texture(inMtlAlbedo[obj.mtlIndex], inUV).xyz, vec3(2.2));
    color = color * albedoMap * mtls.at[obj.mtlIndex].diffuseCoeff;
  }
  
  if(mtls.at[obj.mtlIndex].hasNormalMap == 1) {
    //vec3 normalMap = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 1));
    vec3 normalMap = texture(inMtlNormal[obj.mtlIndex], inUV).xyz;
    normalMap = normalMap * vec3(2.0) - vec3(1.0);
    // do normal mapping, output in 'normal'
    
    mat3 TBN = mat3(tangent, bitan, normal);
    normal = TBN * normalMap;
  }
  
  if(mtls.at[obj.mtlIndex].hasMetallicMap == 1) {
    //float metallicMap  = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 2)).r;
    float metallicMap = texture(inMtlMetallic[obj.mtlIndex], inUV).r;
    metallic = metallicMap * mtls.at[obj.mtlIndex].metallicCoeff;
  }
  
  if(mtls.at[obj.mtlIndex].hasRoughnessMap == 1) {
    //float roughnessMap = texture(inMtlMaps[obj.mtlIndex], vec3(inUV, 3));
    float roughnessMap = texture(inMtlRoughness[obj.mtlIndex], inUV).r;
    roughness = roughnessMap * mtls.at[obj.mtlIndex].roughnessCoeff;
  }
  
  outPos    = vec4(inWorldPosition.xyz, 1);
  outNormal = vec4(normal, metallic);
  outColor  = vec4(color, roughness);
}