#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D logoTexture;

layout (push_constant) uniform PushConstant {
    float aspectRatio;
} push;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(texture(logoTexture, inUV).rgb, 1);
}