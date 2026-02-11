#version 450

layout(location = 0) in vec2 inTexCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D yTexture;
layout(set = 0, binding = 1) uniform sampler2D uvTexture;

void main() {
    float y = texture(yTexture, inTexCoords).r;
    vec2 uv = texture(uvTexture, inTexCoords).rg;

    float u = uv.r - 0.5;
    float v = uv.g - 0.5;

    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;

    outColor = vec4(r, g, b, 1.0);
}
