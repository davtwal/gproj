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
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = inColor;
    
    //vec3 worldNormal = (normalize(inWorldNormal.xyz) + vec3(1,1,1)) / 2;
    //color = worldNormal;

    float d = dot(-normalize(cam.viewDir), normalize(inWorldNormal.xyz));

    outColor = vec4(d * color, 1.0);
}