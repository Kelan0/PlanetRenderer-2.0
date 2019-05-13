#pragma once

#include "core/Core.h"
#include <thread>
#include <mutex>
#include <condition_variable>

class GLMesh;
class MeshData;
class ShaderProgram;
class Planet;
class TerrainQuad;
class InstanceBuffer;
struct VertexLayout;
enum CubeFace;

#define TERRAIN_VERTEX_LAYOUT VertexLayout(8, {        \
	VertexAttribute(0, 2, 0),					       \
}, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.z)}; })

struct TerrainInfo {
	fvec3 debug;
	int32 textureIndex;
	ivec4 neighbourDivisions;
	fvec4 textureCoords;

	fmat4 quadCorners;
	fmat4 quadNormals;
};

struct TerrainRenderTask {
	TerrainQuad* terrainQuad;
	Planet* planet;

	dmat4 faceTransformation;
	dmat4 cameraToScreen;
	dvec2 cameraFacePosition;
};

class TerrainRenderer {
private:
	GLMesh* terrainMesh;
	ShaderProgram* terrainProgram;
	InstanceBuffer* terrainInstanceBuffer;

	std::thread threads[4];
	std::deque<TerrainRenderTask> inputBuffers[4];
	std::vector<TerrainInfo> outputBuffer[4];
	std::condition_variable threadInput;
	std::condition_variable threadOutput;
	std::mutex mut;

	int terrainResolution;

	void doRender(TerrainQuad* terrainQuad, int depth, double r, dvec2 cameraFacePosition, dmat4 localToScreen, dmat4 faceTransformation, std::vector<TerrainInfo>& instances);

	void threadProc(int32 id);
public:
	TerrainRenderer(int terrainResolution);

	~TerrainRenderer();

	void render(Planet* planet, CubeFace face, TerrainQuad* terrainQuad, double partialTicks, double dt);

	MeshData* createTerrainTileMesh();
};

