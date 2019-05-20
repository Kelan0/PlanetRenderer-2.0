#version 330 core

const vec2 histogramMinBound = vec2(0.68, 0.02);
const vec2 histogramMaxBound = vec2(0.98, 0.32);
in vec2 fs_texturePosition;

uniform mat4 invViewProjectionMatrix;
uniform float depthCoefficient;
uniform float nearPlane;
uniform float farPlane;
uniform float scaleFactor;
uniform int msaaSamples;
uniform int histogramBinCount;
uniform int histogramDownsample;
uniform vec2 screenResolution;
uniform vec2 histogramResolution;
uniform float screenExposureMultiplier;
uniform sampler2DMS albedoTexture;
uniform sampler2DMS normalTexture;
uniform sampler2DMS positionTexture;
uniform sampler2DMS specularEmission;
uniform sampler2DMS depthTexture;

uniform sampler2D histogramTexture;
uniform sampler2D histogramCumulativeTexture;

out vec4 outColour;

float linearizeDepth(float reconDepth) {
    float f = farPlane;
    float n = nearPlane;
    float d = reconDepth;

    return f / (f - n) + (f * n / (n - f)) / d;
}

float reconstructDepth(float logDepth) {
    float f = farPlane;

    return pow(2.0, logDepth * log2(f + 1.0)) - 1.0;
}

vec3 reconstructWorldPosition(vec2 uv, float linearDepth) {
    vec4 p = invViewProjectionMatrix * (vec4(uv, linearDepth, 1.0) * 2.0 - 1.0);
    return p.xyz / p.w;
}

float segmentDistanceSq(vec2 a, vec2 b, vec2 p) {
    vec2 a2b = b - a;
    vec2 a2p = p - a;
    float lenSq = dot(a2b, a2b);

    if (lenSq < 1e-12) {
        return dot(a2p, a2p);
    }

    float t = clamp(dot(a2p, a2b) / lenSq, 0.0, 1.0);
    vec2 v = a + t * a2b;
    vec2 v2p = p - v;

    return dot(v2p, v2p);
}

void renderHistogramDebug(inout vec3 colour) {
    vec2 uv;
    uv.x = (histogramMaxBound.x - fs_texturePosition.x) / (histogramMaxBound.x - histogramMinBound.x);
    uv.y = (histogramMaxBound.y - fs_texturePosition.y) / (histogramMaxBound.y - histogramMinBound.y);
    
    vec2 pad = vec2(2.0 / (screenResolution * (histogramMaxBound - histogramMinBound)));
    if (uv.x >= -pad.x && uv.x < 1.0 + pad.x && uv.y >= -pad.y && uv.y < 1.0 + pad.y) {
        colour = clamp(colour, 0.0, 1.0) * 0.66;
        if (uv.x >= 0.0 && uv.x < 1.0 && uv.y >= 0.0 && uv.y < 1.0) {

            vec2 r = floor(screenResolution * (histogramMaxBound -  histogramMinBound));

            float x = 1.0 - uv.x;
            float y = 1.0 - uv.y;

            float x0 = clamp(floor(x * float(histogramBinCount) - 1.0) / float(histogramBinCount), 0.0, 1.0);
            float x1 = clamp(floor(x * float(histogramBinCount) - 0.0) / float(histogramBinCount), 0.0, 1.0);
            
            vec4 histogram = texture(histogramTexture, vec2(x0, 0.0)).rgba / float(histogramResolution.x * histogramResolution.y) * float(20.0);
            vec4 prevCDF = texture(histogramCumulativeTexture, vec2(x0, 0.0));
            vec4 currCDF = texture(histogramCumulativeTexture, vec2(x1, 0.0));

            //vec4 m = (x1 - x0) / max(vec4(0.01), abs(currCDF - prevCDF)); // line slope
            float t = fract((x - x0) / (x1 - x0));
            vec4 interpCDF = (1.0 - t) * prevCDF + t * currCDF;

            vec4 m = max((x1 - x0) / max(currCDF - prevCDF, vec4(1.0 / r.y)), 0.05);
            m.x = m.x < 1.0 ? 1.0 / m.x : m.x;
            m.y = m.y < 1.0 ? 1.0 / m.y : m.y;
            m.z = m.z < 1.0 ? 1.0 / m.z : m.z;
            m.w = m.w < 1.0 ? 1.0 / m.w : m.w;

            vec4 distCDF = vec4( // Distance to the line segment formed by the two end points of the previous CDF and current CDF values.
                abs(interpCDF.r - y) * r.y,//segmentDistanceSq(vec2(x0, prevCDF.r) * r, vec2(x1, currCDF.r) * r, vec2(x, y) * r),
                abs(interpCDF.g - y) * r.y,//segmentDistanceSq(vec2(x0, prevCDF.g) * r, vec2(x1, currCDF.g) * r, vec2(x, y) * r),
                abs(interpCDF.b - y) * r.y,//segmentDistanceSq(vec2(x0, prevCDF.b) * r, vec2(x1, currCDF.b) * r, vec2(x, y) * r),
                abs(interpCDF.a - y) * r.y //segmentDistanceSq(vec2(x0, prevCDF.a) * r, vec2(x1, currCDF.a) * r, vec2(x, y) * r)
            );
            
            //histogram.rgb = histogram.aaa;
            //histogram.rgb = vec3(max(max(histogram.r, histogram.g), histogram.b));
            if (histogram.r > y) colour.r = 1.0;
            if (histogram.g > y) colour.g = 1.0;
            if (histogram.b > y) colour.b = 1.0;

            vec3 lineRGB = vec3(-1.0);

            bool lr = distCDF.r < m.r;
            bool lg = distCDF.g < m.g;
            bool lb = distCDF.b < m.b;
            if (lr || lg || lb) {
                colour.rgb = vec3(0.0);
                if (lr) colour.r = 0.66;// / (1.0 + (distCDF.r * r.y));
                if (lg) colour.g = 0.66;// / (1.0 + (distCDF.g * r.y));
                if (lb) colour.b = 0.66;// / (1.0 + (distCDF.b * r.y));
            }

            //if (lineRGB.x != -1.0) {
            //    colour = lineRGB;
            //}
        }
    }
}

void gammaCorrection(inout vec3 colour) {
    // colour = pow(colour, vec3(1.0 / 2.2));
    vec3 x = max(vec3(0.0), colour - 0.004);
    colour = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

void main(void) {
    ivec2 texelPosition = ivec2(fs_texturePosition * screenResolution);
    vec3 worldPosition = texelFetch(positionTexture, texelPosition, 0).xyz;
    float dist = length(worldPosition / scaleFactor);
    
    vec3 colour = texelFetch(albedoTexture, texelPosition, 0).rgb * screenExposureMultiplier;
    gammaCorrection(colour);
    renderHistogramDebug(colour);

    outColour = vec4(colour, 1.0);
    //outColour = vec4(vec3(1.0 / (1.0 + dist)), 1.0);
}