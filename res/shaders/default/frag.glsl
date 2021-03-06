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
uniform bool showMoisture;
uniform int debugSurfaceRenderMode;

out vec3 outDiffuse;
out vec3 outGlow;
out vec3 outNormal;
out vec2 outSpecularEmission;

vec3 catmullRom(vec3 v1, vec3 v2, vec3 v3, vec3 v4, float s) {
    float s2 = s * s;
    float s3 = s * s * s;

    float f1 = -s3 + 2.0 * s2 - s;
    float f2 = +3.0 * s3 - 5.0 * s2 + 2.0;
    float f3 = -3.0 * s3 + 4.0 * s2 + s;
    float f4 = s3 - s2;

    return (f1 * v1 + f2 * v2 + f3 * v3 + f4 * v4) * 0.5;
}

vec3 getMappedColourGradient(float data) {
	const int tgc = 6;
	const vec3 tg[tgc] = vec3[tgc](
		vec3(0.5, 0.0, 1.0),
		vec3(0.0, 0.0, 1.0),
		vec3(0.0, 1.0, 1.0),
		vec3(0.0, 1.0, 0.0),
		vec3(1.0, 1.0, 0.0),
		vec3(1.0, 0.0, 0.0)
    );

    float f = data * (float(tgc) - 1.0);
    int i0 = int(clamp(f - 1.0, 0.0, float(tgc) - 1.0));
    int i1 = int(clamp(f - 0.0, 0.0, float(tgc) - 1.0));
    int i2 = int(clamp(f + 1.0, 0.0, float(tgc) - 1.0));
    int i3 = int(clamp(f + 2.0, 0.0, float(tgc) - 1.0));
	return catmullRom(tg[i0], tg[i1], tg[i2], tg[i3], fract(f));
}

void main(void) {
    
    //if (debugSurfaceRenderMode == 1) {
    //    outDiffuse = colour.rgb * getMappedColourGradient;
    //}

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