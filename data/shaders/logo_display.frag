#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D logoTexture;

layout (push_constant) uniform PushConstant {
    float aspectRatio;
} push;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 fragColor;

void main() {
    // this is beautiful code
    // for sure
    // yes
    vec2 modifiedUV = (inUV * vec2(2, 3) - vec2(0.5, 1));
    if(modifiedUV.r >= 1 || modifiedUV.r <= 0 || modifiedUV.g >= 1 || modifiedUV.g <= 0)
        fragColor = vec4(1, 1, 1, 1);
    else
        fragColor = vec4(texture(logoTexture, modifiedUV).rgb, 1);
}