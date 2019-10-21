#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inWorldPosition;

layout(location = 0) out vec4 outDepth;

layout(push_constant) uniform Depth {
  layout(offset = 4) float near;
  layout(offset = 8) float far;
} depths;

void main() {
  float depth = gl_FragCoord.z / gl_FragCoord.w;
  depth = (depth - depths.near) / (depths.far - depths.near);
  
  // we want to store the four moments needed for Hamburger 4MSM
  float z1 = depth;
  float z2 = depth * depth;
  float z3 = z2 * depth;
  float z4 = z3 * depth;
  
  outDepth = vec4(z1, z2, z3, z4);
}