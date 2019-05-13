#pragma once

#include "core/Core.h"
#define FN_USE_DOUBLES

class FastNoise;

class TerrainGenerator
{
private:
	FastNoise* noise;

public:
	TerrainGenerator(int32 seed);
	~TerrainGenerator();

	double getElevation(dvec3 sphereVector);

	dvec3 getNormal(dvec3 sphereVector);

	dvec3 getGradient(dvec3 sphereVctor);
};