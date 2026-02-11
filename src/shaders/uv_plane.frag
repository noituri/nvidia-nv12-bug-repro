#version 450

layout (location = 0) in vec3 inColor;
layout (location = 0) out vec2 outFragColor;

void main() {
    mat3x2 conversionWeights = mat3x2(
        -0.1146,  0.5,
        -0.3854, -0.4542,
         0.5,    -0.0458
    );
    vec2 conversionBias = vec2(0.5, 0.5);

    outFragColor = clamp(conversionWeights * inColor + conversionBias, vec2(0.0, 0.0), vec2(1.0, 1.0));
}
