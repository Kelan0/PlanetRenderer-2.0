#version 330 core

const vec3 lightDir = vec3(0.26726, 0.8018, 0.5345); // [1, 3, 2]

in vec3 fs_worldPosition;
in vec3 fs_worldNormal;
in vec3 fs_vertexPosition;
in vec3 fs_vertexNormal;
in vec2 fs_vertexTexture;
in float fs_flogz;

uniform float depthCoefficient;
uniform float nearPlane;
uniform float farPlane;
uniform float scaleFactor;
uniform bool lightingEnabled;

out vec3 outDiffuse;
out vec3 outNormal;
out vec2 outSpecularEmission;

void main(void) {
    vec3 colour = vec3(1.0);
    
    if (lightingEnabled) {
        float nDotL = max(0.2, dot(lightDir, fs_worldNormal));
         colour *= nDotL;
    }

    outDiffuse = colour.rgb;
    //float depth = gl_FragCoord.z;
    //float linearDepth = (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
    //outDiffuse = vec4(linearDepth, linearDepth, linearDepth, 1.0);

    gl_FragDepth = log2(fs_flogz) * depthCoefficient * 0.5;
}