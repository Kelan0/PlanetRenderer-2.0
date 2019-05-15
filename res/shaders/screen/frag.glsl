#version 330 core

const vec2 histogramMinBound = vec2(0.7, 0.7);
const vec2 histogramMaxBound = vec2(0.9, 0.9);
in vec2 fs_texturePosition;

uniform int msaaSamples;
uniform int histogramBinCount;
uniform vec2 resolution;
uniform sampler2DMS albedoTexture;
uniform sampler2DMS normalTexture;
uniform sampler2DMS specularEmission;
uniform sampler2DMS depthTexture;

uniform sampler2D histogramTexture;

out vec4 outColour;

void main(void) {
    ivec2 texelPosition = ivec2(fs_texturePosition * resolution);
    vec3 colour = texelFetch(albedoTexture, texelPosition, 0).rgb;

    
    vec2 uv;
    uv.x = (histogramMaxBound.x - fs_texturePosition.x) / (histogramMaxBound.x - histogramMinBound.x);
    uv.y = (histogramMaxBound.y - fs_texturePosition.y) / (histogramMaxBound.y - histogramMinBound.y);
    
    if (uv.x >= 0.0 && uv.x < 1.0 && uv.y >= 0.0 && uv.y < 1.0) {
        colour = vec3(texture(histogramTexture, vec2(uv.x, 0.0)).rgb);
    }

    outColour = vec4(colour, 1.0);
}