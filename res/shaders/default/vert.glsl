#version 330 core

in vec3 vs_vertexPosition;
in vec3 vs_vertexNormal;
in vec2 vs_vertexTexture;

uniform mat4 modelMatrix;
uniform mat4 normalMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewProjectionMatrix;
uniform float depthCoefficient;
uniform float scaleFactor;

out vec3 fs_worldPosition;
out vec3 fs_worldNormal;
out vec3 fs_vertexPosition;
out vec3 fs_vertexNormal;
out vec2 fs_vertexTexture;
out float fs_flogz;

void main(void) {
    mat4 normalMatrix = transpose(inverse(modelMatrix));
    vec4 worldNormal = normalMatrix * vec4(vs_vertexNormal, 0.0);
    vec4 worldPosition = modelMatrix * vec4(vs_vertexPosition, 1.0);

    fs_worldPosition = worldPosition.xyz;
    fs_worldNormal = worldNormal.xyz;
    fs_vertexPosition = vs_vertexPosition;
    fs_vertexNormal = vs_vertexNormal;
    fs_vertexTexture = vs_vertexTexture;

    vec4 screenPosition = viewProjectionMatrix * worldPosition;
    fs_flogz = 1.0 + screenPosition.w;

    gl_Position = screenPosition;
    gl_Position.z = (log2(max(1e-6, fs_flogz)) * depthCoefficient - 1.0) * screenPosition.w;
}