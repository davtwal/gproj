#version 450

#define SIZE_X 8
#define SIZE_Y 8

layout(constant_id = 0) const float LUMINANCE_FRACTION = .587 / .299;

layout(constant_id = 1) const float FXAA_EDGE_THRESHOLD_MIN = 1.0 / 16.0;
layout(constant_id = 2) const float FXAA_EDGE_THRESHOLD     = 1.0 / 4.0;
layout(constant_id = 3) const float FXAA_SUBPIX_TRIM        = 1.0 / 3.0;
layout(constant_id = 4) const float FXAA_SUBPIX_TRIM_SCALE  = 1.0 / 1.0;
layout(constant_id = 5) const float FXAA_SUBPIX_CAP         = 3.0 / 4.0;
layout(constant_id = 6) const uint  FXAA_SEARCH_STEPS       = 12;
layout(constant_id = 7) const float FXAA_SEARCH_THRESHOLD   = 1.0 / 4.0;

layout(binding = 0) uniform sampler2D image;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

#define FXAA_PC 1
#define FXAA_QUALITY__PRESET 12
#define FXAA_GLSL_130 1
#define FXAA_GREEN_AS_LUMA 1
#include "FXAA.glsl"

//shared vec4 loadedData[SIZE_X + 2][SIZE_Y + 2];

float fxaaLuma(vec3 rgb) {
    return rgb.y * LUMINANCE_FRACTION + rgb.x;
}

void main() { 
    ivec2 size = textureSize(image, 0);
    vec2 du_dv = vec2(1.0 / size.x, 1.0 / size.y);
    vec2 pos = inUV * (size - ivec2(1));
    fragColor = FxaaPixelShader(pos, image, du_dv, 0.75, 0.166, 0.0312);
}