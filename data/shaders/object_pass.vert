#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
} cam;

layout(binding = 1) uniform ObjectUBO {
  mat4 model;
} obj;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUV;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec4 outWorldPosition;
layout(location = 1) out vec4 outWorldNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBitangent;
layout(location = 4) out vec2 outUV;
layout(location = 5) out vec3 outColor;

void main() {
  outWorldPosition  = obj.model * vec4(inPosition, 1.0);
  outWorldNormal    = inverse(transpose(obj.model)) * vec4(inNormal, 0);
  outTangent        = inTangent;
  outBitangent      = inBitangent;
  outUV             = inUV;
  outColor          = inColor;

  gl_Position = cam.proj * cam.view * outWorldPosition;
}