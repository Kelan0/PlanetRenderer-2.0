#version 450 core

const vec3 directionToLight = vec3(0.26726, 0.8018, 0.5345); // [1, 3, 2]

in vec3 fs_debug;
in flat int fs_textureIndex;
in flat ivec4 fs_neighbourDivisions;

in vec2 fs_vertexPosition;
in vec2 fs_texturePosition;
in vec3 fs_screenPosition;
in mat4 fs_quadCorners;
in mat4 fs_quadNormals;
in vec4 fs_interp;

in float fs_flogz;

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

out vec4 outColour;

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
    //vec3 hm10 = getHeightmap(vec2(1.0, 0.0) * f).xyz;
    //vec3 hm01 = getHeightmap(vec2(0.0, 1.0) * f).xyz;
    //vec3 hm00 = getHeightmap(vec2(0.0, 0.0) * f).xyz;

    //vec3 screenNormal = normalize(cross(hm00 - hm10, hm01 - hm00));

    vec3 worldNormal = normalize(texture(heightSampler, vec3(fs_texturePosition, fs_textureIndex)).xyz);

    float nDotL = max(0.0, dot(directionToLight, worldNormal));
    vec3 colour = nDotL * vec3(1.0);

    //vec3 colour = fs_debug;

    if (overlayDebug) {
        vec3 debugColour = vec3(0.0, 1.0, 0.0);
        outColour = vec4(mix(colour, debugColour, (fs_debug[2] * fs_debug[2]) * 0.5), 1.0);
    } else if (showDebug) {
        outColour = vec4(fs_debug, 1.0);
    } else {
        outColour = vec4(colour, 1.0);
    }

    gl_FragDepth = log2(fs_flogz) * depthCoefficient * 0.5;
}