#include "TerrainGenerator.h"
#include "FastNoise/FastNoise.h"


TerrainGenerator::TerrainGenerator(int32 seed) {
	this->noise = new FastNoise(seed);
	this->noise->SetNoiseType(FastNoise::SimplexFractal);
	this->noise->SetFractalOctaves(24);
	this->noise->SetFractalGain(0.8);
}

TerrainGenerator::~TerrainGenerator() {
	delete this->noise;
}

double TerrainGenerator::getElevation(dvec3 sphereVector) {
	return this->noise->GetNoise(sphereVector.x, sphereVector.y, sphereVector.z);
}

dvec3 TerrainGenerator::getNormal(dvec3 sphereVector) {
	return dvec3();
}

dvec3 TerrainGenerator::getGradient(dvec3 sphereVctor) {
	return dvec3();
}
