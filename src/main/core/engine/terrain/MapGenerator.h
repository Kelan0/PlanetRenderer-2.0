#pragma once

#include "core/Core.h"

class Planet;

class MapGenerator;
class MapMesh;

struct MapVertex;
struct MapTriangle;
struct MapCell;

class MapGenerator
{
private:
	Planet* planet;
	MapMesh* mapMesh;

public:
	MapGenerator(Planet* planet, uint32 resolution = 3);

	~MapGenerator();

	void render(double partialTicks, double dt);
};

class MapMesh {
private:
	std::vector<MapCell> cells;
	uint32 resolution;

	std::vector<MapVertex> vertices;
	std::vector<MapTriangle> triangles;

public:
	MapMesh(uint32 resolution);

	void render(double partialTicks, double dt);
};

struct MapVertex {
	glm::dvec3 position;

	MapVertex(double x, double y, double z);

	MapVertex(glm::dvec3 position);
};

struct MapTriangle {
	uvec3 indices;

	MapTriangle(uint32 i0, uint32 i1, uint32 i2);
};

struct MapCell {

};

