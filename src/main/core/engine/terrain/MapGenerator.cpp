#include "MapGenerator.h"
#include "core/application/Application.h"
#include "core/engine/renderer/DebugRenderer.h"
#include "core/engine/geometry/MeshData.h"
#include "core/util/Time.h"



MapGenerator::MapGenerator(Planet* planet, uint32 resolution) {
	this->planet = planet;
	this->mapMesh = new MapMesh(resolution);
}

MapGenerator::~MapGenerator() {
	delete this->mapMesh;
}

void MapGenerator::render(double partialTicks, double dt) {
	this->mapMesh->render(partialTicks, dt);
}

MapMesh::MapMesh(uint32 resolution) {
	const double X = 0.525731112119133606;
	const double Z = 0.850650808352039932;
	const double N = 0.0;



	this->vertices = {
		MapVertex(-X, +N, +Z), MapVertex(+X, +N, +Z), MapVertex(-X, +N, -Z), MapVertex(+X, +N, -Z),
		MapVertex(+N, +Z, +X), MapVertex(+N, +Z, -X), MapVertex(+N, -Z, +X), MapVertex(+N, -Z, -X),
		MapVertex(+Z, +X, +N), MapVertex(-Z, +X, +N), MapVertex(+Z, -X, +N), MapVertex(-Z, -X, +N)
	};

	this->triangles = {
		MapTriangle(1, 4, 0), MapTriangle(4, 9, 0), MapTriangle(4, 5, 9), MapTriangle(8, 5, 4), MapTriangle(1, 8, 4),
		MapTriangle(1, 10, 8), MapTriangle(10, 3, 8), MapTriangle(8, 3, 5), MapTriangle(3, 2, 5), MapTriangle(3, 7, 2),
		MapTriangle(3, 10, 7), MapTriangle(10, 6, 7), MapTriangle(6, 11, 7), MapTriangle(6, 0, 11), MapTriangle(6, 1, 0),
		MapTriangle(10, 1, 6), MapTriangle(11, 0, 9), MapTriangle(2, 11, 9), MapTriangle(5, 2, 9), MapTriangle(11, 2, 7)
	};

	uint64 a = Time::now();
	for (int i = 0; i < resolution; i++) {
		std::vector<MapTriangle> newTriangles;
		newTriangles.reserve(20 * (1 << (i * 2)));

		for (int j = 0; j < this->triangles.size(); j++) {
			MapTriangle tri = this->triangles[j];

			int32 iv0 = tri.indices.x;
			int32 iv1 = tri.indices.y;
			int32 iv2 = tri.indices.z;
			MapVertex v0 = this->vertices[iv0];
			MapVertex v1 = this->vertices[iv1];
			MapVertex v2 = this->vertices[iv2];

			int32 ih0 = this->vertices.size();
			this->vertices.push_back(MapVertex(glm::normalize(v0.position + v1.position)));

			int32 ih1 = this->vertices.size();
			this->vertices.push_back(MapVertex(glm::normalize(v1.position + v2.position)));

			int32 ih2 = this->vertices.size();
			this->vertices.push_back(MapVertex(glm::normalize(v2.position + v0.position)));

			//     v0
			//    /  \
			//   h2 - h0
			//  /  \  / \
			// v2 - h1 - v1

			newTriangles.push_back(MapTriangle(iv0, ih0, ih2));
			newTriangles.push_back(MapTriangle(iv1, ih1, ih0));
			newTriangles.push_back(MapTriangle(iv2, ih2, ih1));
			newTriangles.push_back(MapTriangle(ih0, ih1, ih2));
		}

		this->triangles = newTriangles;
	}

	// Generate hexagons using triangle center points.

	uint64 b = Time::now();

	logInfo("Took %f ms to generate isosphere of resolution %d", (b - a) / 1000000.0, resolution);
}

void MapMesh::render(double partialTicks, double dt) {

	std::vector<Vertex> v;
	std::vector<int32> i;

	for (int idx = 0; idx < this->vertices.size(); idx++) {
		MapVertex vtx = this->vertices[idx];
		v.push_back(Vertex(vtx.position * 6050.0));
	}

	for (int idx = 0; idx < this->triangles.size(); idx++) {
		MapTriangle tri = this->triangles[idx];
		i.push_back(tri.indices.x);
		i.push_back(tri.indices.y);
		i.push_back(tri.indices.y);
		i.push_back(tri.indices.z);
		i.push_back(tri.indices.z);
		i.push_back(tri.indices.x);
	}

	DEBUG_RENDERER.begin(LINES);
	DEBUG_RENDERER.setLineThickness(4.0F);
	DEBUG_RENDERER.setLightingEnabled(false);
	DEBUG_RENDERER.render(v, i);
	DEBUG_RENDERER.finish();
}

MapVertex::MapVertex(double x, double y, double z) :
	position(x, y, z) {}

MapVertex::MapVertex(glm::dvec3 position):
	position(position) {}

MapTriangle::MapTriangle(uint32 i0, uint32 i1, uint32 i2) :
	indices(i0, i1, i2) {}
