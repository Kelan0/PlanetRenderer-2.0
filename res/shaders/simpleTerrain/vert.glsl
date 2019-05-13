#version 450 core

const float eps = 1e-4;

in vec2 vs_vertexPosition;

in float vs_quadSize;
in vec3 vs_debug;
in int vs_textureIndex;
in ivec4 vs_neighbourDivisions;
in vec4 vs_textureCoords;

in mat4 vs_quadCorners;
in mat4 vs_quadNormals;

uniform mat4 screenToLocal;
uniform mat4 localToScreen;

uniform sampler2DArray heightSampler;
uniform int textureSize;
uniform int texturePadding;

uniform float elevationScale;
uniform float planetRadius;
uniform float scaleFactor;
uniform bool renormalizeSphere;

uniform float depthCoefficient;
uniform int debugInt;

out float fs_quadSize;
out vec3 fs_debug;
out flat int fs_textureIndex;

out vec2 fs_vertexPosition;
out vec2 fs_texturePosition;
out vec3 fs_screenPosition;
out mat4 fs_quadCorners;
out mat4 fs_quadNormals;
out vec4 fs_interp;

out float fs_flogz;

vec4 getInterpolatedEdgeHeight(float u, float v, bool flipped) {
    u = clamp(u, 0.0, 1.0);
    float s = 8.0;

    float a = floor(u * s) / s;
    float b = ceil(u * s) / s;

    vec4 edgeHeight;

    if (abs(a - b) > 1e-8) { // a != b
        float interp = (u - a) / (b - a);

        vec2 tp0, tp1;

        if (flipped) {
            tp0 = vec2(v, a);
            tp1 = vec2(v, b);
        } else {
            tp0 = vec2(a, v);
            tp1 = vec2(b, v);
        }

        vec4 hm0 = texture(heightSampler, vec3(vs_textureCoords.xy + tp0 * vs_textureCoords.zw, vs_textureIndex));
        vec4 hm1 = texture(heightSampler, vec3(vs_textureCoords.xy + tp1 * vs_textureCoords.zw, vs_textureIndex));

        edgeHeight = interp * hm0 + (1.0 - interp) * hm1;
    } else {
        edgeHeight = texture(heightSampler, vec3(vs_textureCoords.xy + vs_vertexPosition.xy * vs_textureCoords.zw, vs_textureIndex));
    }

    return edgeHeight;
}

void main(void) {
    vec2 texturePosition = vs_textureCoords.xy + vs_vertexPosition.xy * vs_textureCoords.zw;

    //const bool xPos = vs_vertexPosition.x > (1.0 - eps) && vs_neighbourDivisions[0] < 0;
    //const bool xNeg = vs_vertexPosition.x < eps && vs_neighbourDivisions[2] < 0;
    //const bool yPos = vs_vertexPosition.y > (1.0 - eps) && vs_neighbourDivisions[1] < 0;
    //const bool yNeg = vs_vertexPosition.y < eps && vs_neighbourDivisions[3] < 0;

    vec4 heightmap;
    // if (xPos || xNeg) {
    //     if (xPos) {
    //         heightmap = getInterpolatedEdgeHeight(vs_vertexPosition.y, 1.0, false);
    //     } else if (xNeg) {
    //         heightmap = getInterpolatedEdgeHeight(vs_vertexPosition.y, 0.0, false);
    //     }

    //     if (yPos) {
    //         heightmap = getInterpolatedEdgeHeight(vs_vertexPosition.x, 1.0, true);
    //     } else if (yNeg) {
    //         heightmap = getInterpolatedEdgeHeight(vs_vertexPosition.x, 0.0, true);
    //     }
    // } else {
        heightmap = texture(heightSampler, vec3(texturePosition, vs_textureIndex));
    // }
    
    float height = heightmap.w * elevationScale * scaleFactor; // height is causing cracking

    vec4 uvUV = vec4(vs_vertexPosition.xy, vec2(1.0) - vs_vertexPosition.xy);
    vec4 interp = uvUV.xzzx * uvUV.yyww;
    
    vec4 screenPosition;

    if (renormalizeSphere) {
        vec3 localPosition = normalize((screenToLocal * vs_quadCorners * interp).xyz) * planetRadius * scaleFactor;
        vec3 localNormal = (screenToLocal * vs_quadNormals * interp).xyz;
        localPosition += height * localNormal;
        screenPosition = localToScreen * vec4(localPosition, 1.0);
    } else {
        screenPosition = (vs_quadCorners + height * vs_quadNormals) * interp;// + (height * normalize(vs_quadNormals * interp));
    }
    
    fs_quadSize = vs_quadSize;
    fs_debug = vs_debug;
    fs_textureIndex = vs_textureIndex;
    fs_texturePosition = texturePosition;
    uvUV = vec4(fs_texturePosition.xy, vec2(1.0) - fs_texturePosition.xy);
    interp = uvUV.xzzx * uvUV.yyww;

    fs_vertexPosition = vs_vertexPosition;
    fs_screenPosition = screenPosition.xyz;
    fs_quadCorners = vs_quadCorners;
    fs_quadNormals = vs_quadNormals;
    fs_interp = interp;

    fs_flogz = 1.0 + screenPosition.w;

    gl_Position = screenPosition;
    gl_Position.z = (log2(max(1e-6, fs_flogz)) * depthCoefficient - 1.0) * screenPosition.w;
}