#version 430 core

const vec3 lightDir = vec3(0.26726, 0.8018, 0.5345); // [1, 3, 2]

in vec3 fs_worldPosition;
in vec3 fs_worldNormal;
in vec3 fs_vertexPosition;
in vec3 fs_vertexNormal;
in vec2 fs_vertexTexture;

out vec4 outColour;

void main(void) {
    float nDotL = max(0.0, dot(lightDir, fs_worldNormal));
    vec3 colour = nDotL * vec3(1.0);
    outColour = vec4(colour, 1.0);
}