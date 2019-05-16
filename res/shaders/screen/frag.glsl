#version 330 core

const vec2 histogramMinBound = vec2(0.68, 0.02);
const vec2 histogramMaxBound = vec2(0.98, 0.32);
in vec2 fs_texturePosition;

uniform int msaaSamples;
uniform int histogramBinCount;
uniform int histogramDownsample;
uniform vec2 resolution;
uniform float screenLuminance;
uniform sampler2DMS albedoTexture;
uniform sampler2DMS normalTexture;
uniform sampler2DMS specularEmission;
uniform sampler2DMS depthTexture;

uniform sampler2D histogramTexture;

out vec4 outColour;

void main(void) {
    vec3 colour = texelFetch(albedoTexture, ivec2(fs_texturePosition * resolution), 0).rgb / screenLuminance;
    // vec3 colour = texture(histogramTexture, fs_texturePosition).rgb;

    
    vec2 uv;
    uv.x = (histogramMaxBound.x - fs_texturePosition.x) / (histogramMaxBound.x - histogramMinBound.x);
    uv.y = (histogramMaxBound.y - fs_texturePosition.y) / (histogramMaxBound.y - histogramMinBound.y);
    
    vec2 pad = vec2(2.0 / (resolution * (histogramMaxBound - histogramMinBound)));
    if (uv.x >= -pad.x && uv.x < 1.0 + pad.x && uv.y >= -pad.y && uv.y < 1.0 + pad.y) {
        colour = clamp(colour, 0.0, 1.0) * 0.66;
        if (uv.x >= 0.0 && uv.x < 1.0 && uv.y >= 0.0 && uv.y < 1.0) {
            vec3 histogram = texture(histogramTexture, 1.0 - uv).aaa / (16384.0 / float(histogramDownsample * histogramDownsample));
            //histogram.rgb = vec3(max(max(histogram.r, histogram.g), histogram.b));
            if (histogram.r > 1.0 - uv.y) colour.r = 1.0;
            if (histogram.g > 1.0 - uv.y) colour.g = 1.0;
            if (histogram.b > 1.0 - uv.y) colour.b = 1.0;
        }
    }

    outColour = vec4(colour, 1.0);
}