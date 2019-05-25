#version 330 core

const vec3 directionToLight = vec3(0.26726, 0.8018, 0.5345); // [1, 3, 2]

in vec2 fs_texturePosition;

uniform float scaleFactor;
uniform int msaaSamples;
uniform vec2 screenResolution;
uniform sampler2DMS albedoTexture;
uniform sampler2DMS glowTexture;
uniform sampler2DMS normalTexture;
uniform sampler2DMS positionTexture;
uniform sampler2DMS specularEmission;
uniform sampler2DMS depthTexture;

out vec4 outColour;

void main(void) {
    ivec2 texelPosition = ivec2(fs_texturePosition * screenResolution);
    vec3 worldPosition = texelFetch(positionTexture, texelPosition, 0).xyz / scaleFactor;
    float distSq = dot(worldPosition, worldPosition);
    
    vec3 albedo = texelFetch(albedoTexture, texelPosition, 0).rgb;
    vec3 normal = texelFetch(normalTexture, texelPosition, 0).xyz;

    vec3 colour = albedo;

    // TODO: specular/emission, multiple lights, MSAA
    if (distSq > 0.0) {
        float nDotL = clamp(dot(normal, directionToLight), 0.0, 1.0);
        colour *= nDotL;
    }

    outColour = vec4(colour, 1.0);
}