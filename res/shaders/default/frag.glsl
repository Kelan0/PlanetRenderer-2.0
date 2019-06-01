#version 330 core

const vec3 lightDir = vec3(0.26726, 0.8018, 0.5345); // [1, 3, 2]

in vec3 fs_worldPosition;
in vec3 fs_worldNormal;
in vec3 fs_vertexPosition;
in vec3 fs_vertexNormal;
in vec2 fs_vertexTexture;
in vec3 fs_vertexColour;
in float fs_flogz;

uniform vec4 colour = vec4(1.0);
uniform float depthCoefficient;
uniform float nearPlane;
uniform float farPlane;
uniform float scaleFactor;
uniform bool lightingEnabled;

out vec3 outDiffuse;
out vec3 outGlow;
out vec3 outNormal;
out vec2 outSpecularEmission;

void main(void) {
    outDiffuse = colour.rgb * fs_vertexColour.rgb;

    if (lightingEnabled) {
        float nDotL = max(0.2, dot(lightDir, fs_worldNormal));
         outDiffuse.rgb *= nDotL;
    }

    outNormal = fs_worldNormal;

    //float depth = gl_FragCoord.z;
    //float linearDepth = (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
    //outDiffuse = vec4(linearDepth, linearDepth, linearDepth, 1.0);

    gl_FragDepth = log2(fs_flogz) * depthCoefficient * 0.5;
}