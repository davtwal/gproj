#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
} cam;

layout(binding = 1) uniform sampler2D inGBuffPosition;
layout(binding = 2) uniform sampler2D inGBuffNormal;
layout(binding = 3) uniform sampler2D inGBuffColor;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 inPos = texture(inGBuffPosition, inUV);
    vec4 inNormal = texture(inGBuffNormal,inUV);
    vec4 inColor = texture(inGBuffColor, inUV);
    
    
    fragColor = vec4(inColor.xyz, 1);
}