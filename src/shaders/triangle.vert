#version 450

layout (location = 0) out vec3 outColor;
layout (push_constant) uniform constants {
    float time;
} Consts;

void main() {
    const vec3 vertices[3] = vec3[3](
        vec3(-0.5, 0.0, 0.0),
        vec3(0.5, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0)
    );
    const vec3 colors[3] = vec3[3](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );

    vec4 position = vec4(vertices[gl_VertexIndex], 1.0);
    position.x += sin(Consts.time * 0.125 * 3.1416);
    gl_Position = position;
    outColor = colors[gl_VertexIndex];
}
