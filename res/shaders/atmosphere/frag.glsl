#version 450 core

const int sampleCount = 16;
const float PI = 3.14159265358979323846264338327950288;
const vec3 directionToLight = vec3(0.26726, 0.8018, 0.5345); // [1, 3, 2]

in vec2 fs_vertexPosition;

uniform mat4 invProjectionMatrix;
uniform mat4 invViewMatrix;

//uniform vec3 directionToLight;
uniform vec3 localCameraPosition;
uniform float innerRadius;
uniform float outerRadius;

uniform vec3 invWavelength;
uniform float eSun;         // Kr * ESun
uniform float kr;           // Kr
uniform float km;           // Kr
uniform float scaleDepth;   // The height where the average atmospheric density is found
uniform float g;            // Mie scattering constant

out vec4 outColour;

vec3 getRayDirection() {
    vec2 screenPos = fs_vertexPosition * 2.0 - 1.0; // range [-1 to +1]
	vec4 coord = vec4(screenPos, -1.0, 1.0); // clip space
	coord = invProjectionMatrix * coord; // eye space
	coord = vec4(coord.x, coord.y, 1.0, 0.0);
	coord = invViewMatrix * coord; // world space

	return normalize(coord.xyz);
}
float getMiePhase(float fc, float fcSq, float g, float g2) {
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fcSq) / pow(1.0 + g2 - 2.0 * g * fc, 1.5);
}

float getRayleighPhase(float fcSq) {
	return 0.75 * (1.0 + fcSq);
}

bool getSphereIntersection(vec3 rayOrigin, vec3 rayDir, float radius, inout float near, inout float far) {
	float heightSq = dot(rayOrigin, rayOrigin); // distance squared from ray origin to planet center. Planet is at [0,0,0] in local space
	float b = 2.0 * dot(rayOrigin, rayDir);
	float c = heightSq - (radius * radius);
	float det = b * b - 4.0 * c;
	
	if (det <= 0.0) {
		near = 1.0;
		far = -1.0;
	} else {
        det = sqrt(det);
		near = 0.5 * (-b - det);
		far = 0.5 * (-b + det);
	}
	return far > near;
}

float density(float height) {
	float invScale = 1.0 / ((outerRadius - innerRadius) * scaleDepth);
	//scaling the density curve, such that it crosses the y-axis at 1, and the x-axis at the atmosphere height (fOuterRadius - fInnerRadius)
	float curDens = exp(invScale * (innerRadius - height));
	float minDens = exp(invScale * (innerRadius - outerRadius)); // cache this and pass it as a parameter...
	
	return max(0.0, (curDens - minDens) / (1.0 - minDens));
}

float optic(vec3 start, vec3 end) { //gets the average density between start and end
	vec3 incr = (end - start) / float(sampleCount);
	vec3 point = start + incr * 0.5;
	
	float sum = 0.0;
	for (int i = 0; i < sampleCount; i++) {
		sum += density(length(point)); // length(point) is the distance to the planet center, since the planet is at [0,0,0] in local space
		point += incr;
	}
	
	return sum * (length(incr) / (outerRadius - innerRadius));
}

void checkValidFragment(vec3 rayDir) { //check if this fragment is behind the horizon, or above the atmosphere.
	float n = 0.0;
	float f = 0.0;
	if (getSphereIntersection(localCameraPosition, rayDir, innerRadius, n, f) && f > abs(n)) {
		//if the ray intersects the inner radius (the planet), and the near value is on the other side of the planet, discard this fragment.
		discard;
	}
}

void main(void) {
	float cameraHeight = length(localCameraPosition); // distance between planet center and camera
	float n, f;
	bool aboveAtmosphere = cameraHeight > outerRadius;
	
	vec3 rayDir = getRayDirection();
	
	checkValidFragment(rayDir);
	getSphereIntersection(localCameraPosition, rayDir, outerRadius, n, f);

    if (aboveAtmosphere && n < 0.0) {
        discard;
    }
	
	vec3 sampleStart = (aboveAtmosphere ? localCameraPosition + rayDir * n : localCameraPosition);
	float rayLength = aboveAtmosphere ? f - n : f;
	
	float sampleHeight, sampleDensity;
	float temp;
	
	vec3 sampleIncr = rayDir * (rayLength / float(sampleCount));
	vec3 samplePoint = sampleStart + sampleIncr * 0.5;
	
	vec3 frontColour = vec3(0.0);
	
	for(int i = 0; i < sampleCount; i++) {
		sampleHeight = length(samplePoint); // distance between planet center and sample point
		sampleDensity = density(sampleHeight);
		
		float lightDist = 0.0;
		getSphereIntersection(samplePoint, directionToLight, outerRadius, temp, lightDist);
		
		vec3 u = samplePoint + directionToLight * lightDist;
		float n = (optic(sampleStart, samplePoint) + optic(samplePoint, u)) * (PI * 4.0);
		
		frontColour += sampleDensity * exp(-n * (kr * invWavelength + km));
		samplePoint += sampleIncr;
	}
	
	vec3 atmospherePos = localCameraPosition + rayDir * (aboveAtmosphere ? n : f);
	vec3 c0 = frontColour * (invWavelength * kr * eSun);
	vec3 c1 = max(frontColour * km * eSun, 0.01);
	vec3 t0 = localCameraPosition - atmospherePos;
	
	float g2 = g * g;
	float fc = dot(directionToLight, t0) / length(t0);
	float fcSq = fc * fc;
	vec3 colour = getRayleighPhase(fcSq) * c0 + getMiePhase(fc, fcSq, g, g2) * c1;

	outColour = vec4(colour, 1.0);
    gl_FragDepth = 0.999999;
}