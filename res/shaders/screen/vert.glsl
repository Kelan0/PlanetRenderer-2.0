#version 330 core

in vec2 vs_vertexPosition;

out vec2 fs_texturePosition;

void main(void) {
    fs_texturePosition = vs_vertexPosition;

    gl_Position = vec4(vs_vertexPosition * 2.0 - 1.0, 0.0, 1.0);
}