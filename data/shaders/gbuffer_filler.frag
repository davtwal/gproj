#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  vec3 eye;
  vec3 viewDir;
} cam;

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
    outPos    = vec4(inWorldPosition.xyz, 1.0);
    
    // do normal mapping
    outNormal = vec4(normalize(inWorldNormal.xyz), 1.0);
    
    // do texturing
    outColor  = vec4(inColor.xyz, 100.0);
}