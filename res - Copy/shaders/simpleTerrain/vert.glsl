#version 430 core

in vec2 vs_vertexPosition;

in vec2 vs_terrainPosition;
in float vs_terrainSize;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewProjectionMatrix;

out vec3 fs_worldPosition;
out vec2 fs_vertexPosition;
out vec2 fs_terrainPosition;
out float fs_terrainSize;

void main(void) {
    vec3 terrainPosition = vec3(vs_terrainPosition * vs_terrainSize + vs_vertexPosition, 0.0); // z axis is up for convenience
    vec3 worldPosition = terrainPosition.xzy; // y axis is up for rendering

    fs_worldPosition = worldPosition;
    fs_vertexPosition = vs_vertexPosition;
    fs_terrainPosition = vs_terrainPosition;
    fs_terrainSize = vs_terrainSize;

    gl_Position = viewProjectionMatrix * vec4(worldPosition, 1.0);
}