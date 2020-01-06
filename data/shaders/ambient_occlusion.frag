#version 450

#include "inc/defines.glsl"

layout(constant_id = 0) const uint NUM_SAMPLES = 15;
layout(constant_id = 1) const float DELTA = 0.001;
layout(constant_id = 2) const float RANGE = 10.0;

layout(binding = 0) uniform sampler2D inGBuffPosition;
layout(binding = 1) uniform sampler2D inGBuffNormal;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

#define PI 3.1415926538

float HeavisideStep(float value) {
    if(value < 0)
        return 0;
    return 1;
}

void main() {
    vec4 sampledPosition = texture(inGBuffPosition, inUV);
    vec4 sampledNormal = texture(inGBuffNormal, inUV);
    
    vec3 P = sampledPosition.xyz;
    vec3 N = normalize(sampledNormal.xyz);

    // S = 2 * pi * c
    float c = 0.1 * RANGE;
    float coeff = 1.0 / NUM_SAMPLES;
    float sum = 0;
    int xprime = int(gl_FragCoord.x);
    int yprime = int(gl_FragCoord.y);
    float phi = (30 * xprime ^ yprime) + 10 * xprime * yprime;
    float d = gl_FragCoord.z / gl_FragCoord.w;
    
    for(uint i = 0; i < NUM_SAMPLES; ++i) {
        float alpha = (i + 0.5) / NUM_SAMPLES;
        float h = alpha * RANGE / d;
        float theta = 2 * PI * alpha * (7.0 * NUM_SAMPLES / 9.0) + phi;

        vec3 point = texture(inGBuffPosition, inUV + h * cos(theta) * sin(theta)).xyz;

        vec3 w = point - P;

        float numerator = max(0, dot(N,w) - DELTA * d) * HeavisideStep(RANGE - length(w));
        float denom = max(c * c, dot(w, w));
        sum += coeff * (numerator / denom);
    }

    float S = 2 * PI * c * sum;

    float s = 1;
    float k = 1;
    float value = pow(max((1 - s * S), 0), k);
    fragColor = vec4(value.xxx, 1);
}