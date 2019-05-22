#version 430 core

writeonly uniform image2DArray tileTexture;
uniform int textureIndex;
uniform float planetRadius;
uniform float elevationScale;
uniform mat4 quadNormals;
uniform mat4 quadCorners;

//	Simplex 3D Noise
//	by Ian McEwan, Ashima Arts
//
vec4 permute(vec4 x) { return mod(((x * 34.0) + 1.0) * x, 289.0); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    //  x0 = x0 - 0. + 0.0 * C
    vec3 x1 = x0 - i1 + 1.0 * C.xxx;
    vec3 x2 = x0 - i2 + 2.0 * C.xxx;
    vec3 x3 = x0 - 1. + 3.0 * C.xxx;

    // Permutations
    i = mod(i, 289.0);
    vec4 p = permute(permute(permute(i.z + vec4(0.0, i1.z, i2.z, 1.0)) + i.y + vec4(0.0, i1.y, i2.y, 1.0)) + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    // Gradients
    // ( N*N points uniformly over a square, mapped onto an octahedron.)
    float n_ = 1.0 / 7.0; // N=7
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z); //  mod(p,N*N)

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_); // mod(j,N)

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    // Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}

vec4 getInterp(vec2 pos) {
    vec4 uvUV = vec4(pos.xy, vec2(1.0) - pos.xy);
    return uvUV.xzzx * uvUV.yyww;
}

float getNoise(vec3 coord) {
    int i = 0;

    float frequency = 2.0;
    float lacunarity = 2.0;
    float gain = 0.5;
    float amplitude = 1.0;
    int octaves = 15;

    coord *= frequency;

    float noiseSum = 0.0;
    while (++i < octaves) {
        coord *= lacunarity;
        amplitude *= gain;
        
        noiseSum += snoise(coord) * amplitude;
    }

    return abs(noiseSum);
}

vec3 getSurfacePosition(float height, vec3 n, vec4 interp) {
    return n + n * (elevationScale / planetRadius) * height;
    //return (quadCorners * interp).xyz;
}

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main(void) {
    ivec3 storePos = ivec3(gl_GlobalInvocationID.xy, textureIndex);
    vec2 quadPosition = vec2(gl_GlobalInvocationID.xy) / vec2(gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);

    float f = 1.0 / 64.0;
    vec4 i00 = getInterp(quadPosition);
    vec4 i10 = getInterp(quadPosition + vec2(f, 0.0));
    vec4 i01 = getInterp(quadPosition + vec2(0.0, f));
    
    vec3 s00 = normalize((quadNormals * i00).xyz);
    vec3 s10 = normalize((quadNormals * i10).xyz);
    vec3 s01 = normalize((quadNormals * i01).xyz);

    float n00 = getNoise(s00);
    float n10 = getNoise(s10);
    float n01 = getNoise(s01);
    
    vec3 normal;

    //if (1.0 - abs(dot(s00, s10)) > 1e-18 && 1.0 - abs(dot(s00, s01)) > 1e-18) {
        vec3 h00 = getSurfacePosition(n00, s00, i00);//((quadCorners + n00 * elevationScale * quadNormals) * i00).xyz;
        vec3 h10 = getSurfacePosition(n10, s10, i10);//((quadCorners + n10 * elevationScale * quadNormals) * i10).xyz;
        vec3 h01 = getSurfacePosition(n01, s01, i01);//((quadCorners + n01 * elevationScale * quadNormals) * i01).xyz;

        normal = cross(normalize(h10 - h00), normalize(h00 - h01));
    //} else {
    //    normal = s00;
    //}

    imageStore(tileTexture, storePos, vec4(normal, n00));
}