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
struct LifeZone;

class MapGenerator
{
private:
	Planet* planet;

	uint32 resolution;

	GLMesh* debugSurfaceTriangleMesh;
	GLMesh* debugSurfaceLineMesh;
	GLMesh* debugCurrentTriangleMesh;
	GLMesh* debugCurrentLineMesh;
	GLMesh* debugClosestWalkMesh;

	std::vector<MapNode*> nodes;
	std::vector<MapEdge*> edges;
	std::vector<MapFace*> faces;

	std::vector<LifeZone*> lifeZones; // List of all lifezones.

	std::vector<int32> debugClosestWalk;

	bool renderDebugCurrents;
	bool renderDebugSurface;
	int debugSurfaceRenderMode;

	void generateIcosohedron();

	void generateAirCurrents();

	void initializeHeat();

	void initializeMoisture();

	void initializeBiomes();

	void generateDebugMeshes();

public:
	MapGenerator(Planet* planet, uint32 resolution = 100);

	~MapGenerator();

	void render(double partialTicks, double dt);

	MapNode* getClosestMapNode(dvec3 point, int startPoint = -1);

	void setRenderDebugCurrents(bool renderDebugCurrents);

	bool doRenderDebugCurrents() const;

	void setRenderDebugSurface(bool renderDebugSurface);

	bool doRenderDebugSurface() const;

	void setDebugSurfaceRenderMode(int renderMode);

	int getDebugSurfaceRenderMode() const;
};

struct MapNode {
	fvec3 p; // The position of this node.
	std::vector<int32> e; // The edges connected to this node.
	std::vector<int32> f; // The faces connected to this node.
	std::vector<float> c; // The air current strengths to each connected node.
	int32 index;

	LifeZone* lifeZone;

	fvec4 heightmapData;

	float area = 1.0;

	bool water;

	float heatAbsorbsion; // The amount of heat that this node can absorb, determined by air speed and if it is water.
	float nextHeat; // The air heat for the next iteration
	float airHeat; // The heat being moved around by wind
	float temperature; // The heat that has settled on (been absorbed by) this node.

	float maxMoisture;
	float moistureAbsorbsion;
	float nextMoisture; // The air moisture for the next iteration
	float airMoisture; // The moisture being moved around by wind
	float moisture; // The moisture that has settled on (been absorbed by) this node.

	float windStrength;
	float normalizedWindStrength;
	fvec3 windVector;

	MapNode(fvec3 p, std::vector<int32> e = {}, std::vector<int32> f = {}) :
		p(p), e(e), f(f), lifeZone(NULL), water(false), temperature(0.0), moisture(0.0), windStrength(0.0), windVector(0.0) {}

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

struct LifeZone {
	std::string name;
	double minTemperature;
	double maxTemperature;
	double minMoisture;
	double maxMoisture;

	fvec3 colour;

	LifeZone(std::string name, double minTemperature, double maxTemperature, double minMoisture, double maxMoisture, fvec3 colour) :
		name(name), minTemperature(minTemperature), maxTemperature(maxTemperature), minMoisture(minMoisture), maxMoisture(maxMoisture), colour(colour) {}
};
