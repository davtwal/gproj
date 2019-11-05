#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "inc/defines.glsl"
#include "inc/lighting.glsl"

layout(binding = 0) uniform ShadowLights {
  ShadowLight at[MAX_GLOBAL_LIGHTS];
} lights;

layout(binding = 1) uniform ObjectUBO {
  mat4 model;
} obj;

layout(push_constant) uniform LightIndexPush {
  int index;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUV;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec4 outWorldPosition;

void main() {
  outWorldPosition  = obj.model * vec4(inPosition, 1.0);
  
  ShadowLight light = lights.at[push.index];
  gl_Position = light.proj * light.view * outWorldPosition;
}