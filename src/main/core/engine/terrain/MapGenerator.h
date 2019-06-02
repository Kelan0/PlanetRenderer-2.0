#pragma once

#include "core/Core.h"

class Planet;
class GLMesh;

class MapGenerator;
class MapMesh;

struct MapVertex;
struct MapTriangle;
struct MapCell;

struct MapNode;
struct MapEdge;
struct MapFace;

class MapGenerator
{
private:
	Planet* planet;

	uint32 resolution;

	GLMesh* debugSurfaceTriangleMesh;
	GLMesh* debugSurfaceLineMesh;
	GLMesh* debugCurrentTriangleMesh;
	GLMesh* debugCurrentLineMesh;

	std::vector<MapNode*> nodes;
	std::vector<MapEdge*> edges;
	std::vector<MapFace*> faces;

	bool renderDebugCurrents;
	bool renderDebugSurface;

	void generateIcosohedron();

	void generateAirCurrents();

	void initializeHeat();

	void generateDebugMeshes();

public:
	MapGenerator(Planet* planet, uint32 resolution = 80);

	~MapGenerator();

	void render(double partialTicks, double dt);

	void setRenderDebugCurrents(bool renderDebugCurrents);

	bool doRenderDebugCurrents() const;

	void setRenderDebugSurface(bool renderDebugSurface);

	bool doRenderDebugSurface() const;
};

struct MapNode {
	dvec3 p; // The position of this node.
	std::vector<int32> e; // The edges connected to this node.
	std::vector<int32> f; // The faces connected to this node.
	std::vector<double> c; // The air current strengths to each connected node.

	fvec4 heightmapData;

	double area = 1.0;

	bool water;
	double heatAbsorbsion; // The amount of heat that this node can absorb, determined by air speed and if it is water.
	double nextHeat; // The air heat for the next iteration
	double airHeat; // The heat being moved around by wind
	double airMoisture; // The moisture being moved around by wind
	double temperature; // The heat that has settled on (been absorbed by) this node.
	double moisture; // The moisture that has settled on (been absorbed by) this node.
	double windStrength;
	double normalizedWindStrength;
	dvec3 windVector;

	MapNode(dvec3 p, std::vector<int32> e = {}, std::vector<int32> f = {}) :
		p(p), e(e), f(f), water(false), temperature(0.0), moisture(0.0), windStrength(0.0), windVector(0.0) {}

	inline bool operator==(const MapNode& node) {
		constexpr double eps = 1e-12;

		if (glm::length2(this->p - node.p) >= eps * eps) return false;
		if (this->e != node.e) return false;
		if (this->f != node.f) return false;
		return true;
	}

	inline bool operator!=(const MapNode& node) {
		return !(*this == node);
	}
};

struct MapEdge {
	std::vector<int32> n; // The nodes that make up this edge.
	std::vector<int32> f; // The faces connected to this edge.

	std::vector<int32> sn;
	std::vector<int32> se;

	MapEdge(std::vector<int32> n, std::vector<int32> f = {}) :
		n(n), f(f) {}

	inline bool operator==(const MapEdge& node) {
		if (this->n != node.n) return false;
		if (this->f != node.f) return false;
		return true;
	}

	inline bool operator!=(const MapEdge& node) {
		return !(*this == node);
	}
};

struct MapFace {
	std::vector<int32> n; // The nodes that make up this face.
	std::vector<int32> e; // The edges that make up this face.

	MapFace(std::vector<int32> n, std::vector<int32> e) :
		n(n), e(e) {}

	inline bool operator==(const MapFace& node) {
		if (this->n != node.n) return false;
		if (this->e != node.e) return false;
		return true;
	}

	inline bool operator!=(const MapFace& node) {
		return !(*this == node);
	}
};

