#version 450 core

const int viewraySampleCount = 16;
const int lightraySampleCount = 8;

const float PI = 3.14159265358979323846264338327950; // 32 d.p

in vec2 fs_vertexPosition;

uniform mat4 invProjectionMatrix;
uniform mat4 invViewMatrix;

uniform vec3 localCameraPosition;
uniform float innerRadius;
uniform float outerRadius;
uniform float rayleighHeight;
uniform float mieHeight;
uniform float sunIntensity;
uniform vec3 sunDirection;
uniform vec3 rayleighWavelength;
uniform vec3 mieWavelength;

out vec4 outColour;

vec3 getRayDirection() {
    vec2 screenPos = fs_vertexPosition * 2.0 - 1.0; // range [-1 to +1]
	vec4 coord = vec4(screenPos, -1.0, 1.0); // clip space
	coord = invProjectionMatrix * coord; // eye space
	coord = vec4(coord.x, coord.y, 1.0, 0.0);
	coord = invViewMatrix * coord; // world space

	return normalize(coord.xyz);
}

bool getSphereIntersection(vec3 rayOrigin, vec3 rayDir, float radius, inout float viewrayNear, inout float viewrayFar) {
	float heightSq = dot(rayOrigin, rayOrigin); // distance squared from ray origin to planet center. Planet is at [0,0,0] in local space
	float b = 2.0 * dot(rayOrigin, rayDir);
	float c = heightSq - (radius * radius);
	float det = b * b - 4.0 * c;
	
	if (det <= 0.0) {
		viewrayNear = 1.0;
		viewrayFar = -1.0;
	} else {
        det = sqrt(det);
		viewrayNear = 0.5 * (-b - det);
		viewrayFar = 0.5 * (-b + det);
	}
	return viewrayFar > viewrayNear;
}

void main(void) {
	vec3 rayOrig = localCameraPosition;
	vec3 rayDir = getRayDirection();

	bool insideAtmosphere = dot(rayOrig, rayOrig) < outerRadius * outerRadius;

	float viewrayNear = 0.0;
	float viewrayFar = 0.0;

	if (!getSphereIntersection(rayOrig, rayDir, outerRadius, viewrayNear, viewrayFar) || viewrayFar <= 0.0 || (!insideAtmosphere && abs(viewrayNear) > viewrayFar)) {
		discard;
	}

	//float innerNear = 0.0;
	//float innerFar = 0.0;
	//if (getSphereIntersection(rayOrig, rayDir, innerRadius, innerNear, innerFar) && innerNear > 0.0) {
	//	viewrayFar = innerNear;
	//}

	if (insideAtmosphere) {
		viewrayNear = 0.0;
	}
	
	const float viewraySegmentLength = (viewrayFar - viewrayNear) / float(viewraySampleCount);

	const float g = 0.76;
	const float mu = dot(rayDir, sunDirection);
	const float rayleighPhase = 3.0 / (16.0 * PI) * (1.0 + mu * mu);
	const float miePhase = 3.0 / (8.0 * PI) * ((1.0 - g * g) * (1.0 + mu * mu)) / ((2.0 + g * g) * pow(1.0 + g * g - 2.0 * g * mu, 1.5));

	vec3 rayleighSum = vec3(0.0);
	vec3 mieSum = vec3(0.0);
	vec3 viewraySamplePoint;
	float viewraySampleHeight;
	float viewrayRayleighOptic = 0.0;
	float viewrayMieOptic = 0.0;
	float viewrayCurrSample = viewrayNear;
	float hr, hm;

	vec3 lightraySamplePoint;
	float lightraySampleHeight;
	float lightrayRayleighOptic;
	float lightrayMieOptic;
	float lightrayCurrSample;
	float lightraySegmentLength;
	float lightrayNear;
	float lightrayFar;

	int i, j;
	
	for (i = 0; i < viewraySampleCount; i++) {
		viewraySamplePoint = rayOrig + rayDir * (viewrayCurrSample + viewraySegmentLength * 0.5);
		viewraySampleHeight = length(viewraySamplePoint) - innerRadius;

		hr = exp(-viewraySampleHeight / rayleighHeight) * viewraySegmentLength;
		hm = exp(-viewraySampleHeight / mieHeight) * viewraySegmentLength;

		viewrayRayleighOptic += hr;
		viewrayMieOptic += hm;

		lightrayNear = 0.0;
		lightrayFar = 0.0;
		getSphereIntersection(viewraySamplePoint, sunDirection, outerRadius, lightrayNear, lightrayFar);
		// Assuming this intersection happens. It should in most cases.

		lightraySegmentLength = lightrayFar / float(lightraySampleCount);
		lightrayCurrSample = 0.0;
		lightrayRayleighOptic = 0.0;
		lightrayMieOptic = 0.0;

		for (j = 0; j < lightraySampleCount; j++) {
			lightraySamplePoint = viewraySamplePoint + sunDirection * (lightrayCurrSample + lightraySegmentLength * 0.5);
			lightraySampleHeight = length(lightraySamplePoint) - innerRadius;

			if (lightraySampleHeight < 0.0) {
				break; // TODO: use terrain height values to determine if ray is under terrain surface. Decode position from depth buffer.
			}

			// TODO: precompute these optical depth values and send them via uniforms.
			lightrayRayleighOptic += exp(-lightraySampleHeight / rayleighHeight) * lightraySegmentLength;
			lightrayMieOptic += exp(-lightraySampleHeight / mieHeight) * lightraySegmentLength;
			lightrayCurrSample += lightraySegmentLength;
		}

		if (j == lightraySampleCount) {
			vec3 tau = rayleighWavelength * (viewrayRayleighOptic + lightrayRayleighOptic) + 1.1 * mieWavelength * (viewrayMieOptic + lightrayMieOptic);
			vec3 att = exp(-tau);

			rayleighSum += att * hr;
			mieSum += att * hm;
		}

		viewrayCurrSample += viewraySegmentLength;
	}
	vec3 colour = (rayleighSum * rayleighWavelength * rayleighPhase + mieSum * mieWavelength * miePhase) * 22.0;
	outColour = vec4(vec3(colour), 1.0);
    gl_FragDepth = 0.999999;
}