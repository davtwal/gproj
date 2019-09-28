#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 outUV;

void main() {
  // 0: bottom left   (-1, -1)
  // 1: bottom right  ( 1, -1)
  // 2: top right     ( 1,  1)
  // 3: top left      (-1,  1)

  gl_Position = vec4(gl_VertexIndex % 3 == 0 ? -1 : 1, gl_VertexIndex < 2 ? -1 : 1, 0, 1);
  outUV = vec2(1, 1);
}