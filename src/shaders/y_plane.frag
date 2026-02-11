#version 450

layout (location = 0) in vec3 inColor;
layout (location = 0) out float outFragColor;

void main() {
    const vec3 conversionWeights = vec3(0.2126, 0.7152, 0.0722);
    outFragColor = clamp(dot(inColor, conversionWeights), 0.0, 1.0);
}
