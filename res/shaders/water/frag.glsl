#version 450 core

in vec3 fs_debug;
in flat int fs_textureIndex;
in flat ivec4 fs_neighbourDivisions;

in vec2 fs_vertexPosition;
in vec2 fs_texturePosition;
in vec4 fs_worldPosition;
in mat4 fs_quadCorners;
in mat4 fs_quadNormals;
in vec4 fs_interp;

in float fs_flogz;

uniform mat4 invViewProjectionMatrix;
uniform mat4 screenToLocal;
uniform mat4 localToScreen;

uniform float depthCoefficient;
uniform float nearPlane;
uniform float farPlane;
uniform float elevationScale;
uniform float planetRadius;
uniform float scaleFactor;
uniform bool renormalizeSphere;
uniform sampler2DArray heightSampler;

uniform bool overlayDebug;
uniform bool showDebug;

out vec3 outDiffuse;
out vec3 outNormal;
out vec3 outPosition;
out vec3 outSpecularEmission;
out vec3 outGlow;

vec4 getInterp(vec2 pos) {
    vec4 uvUV = vec4(pos.xy, vec2(1.0) - pos.xy);
    return uvUV.xzzx * uvUV.yyww;
}

vec4 getHeightmap(vec2 offset) {
    vec2 pos = fs_texturePosition + offset;
    vec4 heightmap = texture(heightSampler, vec3(pos, fs_textureIndex)) * elevationScale * scaleFactor;
    vec4 interp = getInterp(pos);

    if (renormalizeSphere) {
        vec4 localPosition = screenToLocal * fs_quadCorners * interp;
        vec4 localNormal = screenToLocal * fs_quadNormals * interp;
        localPosition.xyz = normalize(localPosition.xyz) * planetRadius * scaleFactor;
        localNormal.xyz = normalize(localNormal.xyz);

        localPosition.xyz += heightmap.w * localNormal.xyz;
        heightmap.xyz = (localToScreen * localPosition).xyz;
    } else {
        heightmap.xyz = ((fs_quadCorners + heightmap.w * fs_quadNormals) * interp).xyz;
    }

    return heightmap;
}

void main(void) {
    float f = 1.0 / 64.0;

    vec3 worldNormal = normalize((screenToLocal * fs_quadNormals * fs_interp).xyz);
    //vec3 worldPosition = (screenToLocal * fs_quadCorners * fs_interp).xyz;

    float depth = pow(min(abs(texture(heightSampler, vec3(fs_texturePosition, fs_textureIndex)).a) * 6.0, 1.0), 1.0);

    vec3 colour = vec3(0.2, 0.3, 0.7) * (1.0 - depth * 0.3);

    float specularPower = 50.0;
    float specularBrightness = 22.0;
    
    if (overlayDebug) {
        vec3 debugColour = vec3(0.0, 1.0, 0.0);
        outDiffuse = mix(colour, debugColour, (fs_debug[2] * fs_debug[2]) * 0.5).rgb;
    } else if (showDebug) {
        outDiffuse = fs_debug.rgb;
    } else {
        outDiffuse = colour.rgb;
    }
    
    outPosition = fs_worldPosition.xyz;
    outNormal = worldNormal;
    outSpecularEmission = vec3(specularPower, specularBrightness, 0.0);
    

    //gl_FragDepth = log2(fs_flogz) * depthCoefficient * 0.5;
}