#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
} cam;

layout(push_constant) uniform PushConstant {
  mat4 model;
} obj;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec4 outWorldPosition;
layout(location = 1) out vec4 outWorldNormal;
layout(location = 2) out vec3 outColor;

void main() {
  outWorldPosition = obj.model * vec4(inPosition, 1.0);
  outWorldNormal = inverse(transpose(obj.model)) * vec4(inNormal, 0);
  outColor = inColor.xyz;

  gl_Position = cam.proj * cam.view * outWorldPosition;
}