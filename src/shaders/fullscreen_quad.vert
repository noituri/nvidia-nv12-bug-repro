#version 450

layout (location = 0) out vec2 outTexCoords;

void main() {
    const vec3 vertices[3] = vec3[3](
        vec3(-3.0, 1.0, 0.0),
        vec3(1.0, -3.0, 0.0),
        vec3(1.0, 1.0, 0.0)
    );
    const vec2 texCoords[3] = vec2[3](
        vec2(-1.0, 0.0),
        vec2(1.0, 2.0),
        vec2(1.0, 0.0)
    );

    gl_Position = vec4(vertices[gl_VertexIndex], 1.0f);
    outTexCoords = texCoords[gl_VertexIndex];
}
