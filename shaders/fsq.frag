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
    vec3 inPos = texture(inGBuffPosition, inUV).xyz;
    vec3 inNormal = normalize(texture(inGBuffNormal, inUV).xyz);
    vec3 inColor = texture(inGBuffColor, inUV).xyz;
    
    vec3 posToEye = cam.eye - inPos;
    
    float d = dot(normalize(posToEye), inNormal);
    fragColor = vec4(d * inColor, 1);
}