#version 450 core

in vec2 vs_vertexPosition;

out vec2 fs_vertexPosition;

void main(void) {
    fs_vertexPosition = vs_vertexPosition.xy;

    gl_Position = vec4(vs_vertexPosition.xy * 2.0 - 1.0, 0.0, 1.0);
}