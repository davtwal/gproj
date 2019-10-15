#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inWorldPosition;

layout(location = 0) out vec4 outDepth;

void main() {
  float d = gl_FragCoord.z / gl_FragCoord.w;
  outDepth = vec4(d, d * d, d * d * d, d * d * d * d);
}