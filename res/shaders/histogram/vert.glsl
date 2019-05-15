#version 330 core

in vec2 vs_vertexPosition;

uniform sampler2DMS textureSampler;
uniform vec2 textureSize;
uniform int binCount;

void main(void) {
    vec3 colour = texelFetch(textureSampler, ivec2(vs_vertexPosition * textureSize), 0).rgb;
    float grayscale = dot(colour, vec3(0.299, 0.587, 0.114));
    float histogramPosition = grayscale * 2.0 - 1.0 + 0.5 / binCount; // range [-1 to +1], -1 = histogram left edge, +1 = histogram right edge.
    
    gl_Position = vec4(histogramPosition, -1.0, 0.0, 1.0);
}