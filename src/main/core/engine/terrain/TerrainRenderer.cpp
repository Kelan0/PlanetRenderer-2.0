#include "TerrainRenderer.h"
#include "core/engine/terrain/TerrainQuad.h"
#include "core/engine/terrain/TileSupplier.h"
#include "core/engine/terrain/Planet.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/scene/GameObject.h"
#include "core/engine/scene/bounding/BoundingVolume.h"
#include "core/engine/renderer/Camera.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/application/Application.h"
#include "core/util/Time.h"
#include "core/util/InputHandler.h"
#include <GL/glew.h>

TerrainQuad* pickedQuad = NULL;

TerrainRenderer::TerrainRenderer(int terrainResolution):
	terrainResolution(terrainResolution) {

	this->terrainProgram = new ShaderProgram();
	this->terrainProgram->addShader(GL_VERTEX_SHADER, "simpleTerrain/vert.glsl");
	this->terrainProgram->addShader(GL_FRAGMENT_SHADER, "simpleTerrain/frag.glsl");

	this->terrainProgram->addAttribute(0, "vs_vertexPosition");
	this->terrainProgram->addAttribute(1, "vs_debug");
	this->terrainProgram->addAttribute(2, "vs_textureIndex");
	this->terrainProgram->addAttribute(3, "vs_neighbourDivisions");
	this->terrainProgram->addAttribute(4, "vs_textureCoords");
	this->terrainProgram->addAttribute(5, "vs_quadCorners");
	this->terrainProgram->addAttribute(9, "vs_quadNormals");

	this->terrainProgram->completeProgram();

	this->terrainMesh = new GLMesh(this->createTerrainTileMesh(), TERRAIN_VERTEX_LAYOUT);
	this->terrainInstanceBuffer = new InstanceBuffer(sizeof(TerrainInfo), 4096, 1, {

		InstanceAttribute(1, 3, GL_FLOAT, offsetof(TerrainInfo, debug)),
		InstanceAttribute(2, 1, GL_INT, offsetof(TerrainInfo, textureIndex)),
		InstanceAttribute(3, 4, GL_INT, offsetof(TerrainInfo, neighbourDivisions)),
		InstanceAttribute(4, 4, GL_FLOAT, offsetof(TerrainInfo, textureCoords)),

		InstanceAttribute(5, 4, GL_FLOAT, offsetof(TerrainInfo, quadCorners) + sizeof(vec4) * 0),
		InstanceAttribute(6, 4, GL_FLOAT, offsetof(TerrainInfo, quadCorners) + sizeof(vec4) * 1),
		InstanceAttribute(7, 4, GL_FLOAT, offsetof(TerrainInfo, quadCorners) + sizeof(vec4) * 2),
		InstanceAttribute(8, 4, GL_FLOAT, offsetof(TerrainInfo, quadCorners) + sizeof(vec4) * 3),

		InstanceAttribute(9, 4, GL_FLOAT, offsetof(TerrainInfo, quadNormals) + sizeof(vec4) * 0),
		InstanceAttribute(10, 4, GL_FLOAT, offsetof(TerrainInfo, quadNormals) + sizeof(vec4) * 1),
		InstanceAttribute(11, 4, GL_FLOAT, offsetof(TerrainInfo, quadNormals) + sizeof(vec4) * 2),
		InstanceAttribute(12, 4, GL_FLOAT, offsetof(TerrainInfo, quadNormals) + sizeof(vec4) * 3),
	});

	int32 maxAttribs;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);

	logInfo("Maximum allowed vertex attribute locations is %d", maxAttribs);

	uint32 threadCount = 1;

	for (int i = 0; i < threadCount; i++) {
		this->threads[i] = std::thread(&TerrainRenderer::threadProc, this, i);
		inputBuffers[i] = std::deque<TerrainRenderTask>();
		outputBuffer[i] = std::vector<TerrainInfo>();
	}
}

TerrainRenderer::~TerrainRenderer() {
	delete this->terrainMesh;
	delete this->terrainProgram;
	delete this->terrainInstanceBuffer;
}

int debug = 0;

void TerrainRenderer::render(Planet* planet, CubeFace face, TerrainQuad* terrainQuad, double partialTicks, double dt) {

	uint64 t0 = Time::now();

	Camera* camera = SCENE_GRAPH.getCamera();
	int32 w, h; Application::getWindowSize(&w, &h);
	fvec2 mouse = (INPUT_HANDLER.getMousePosition() / fvec2(w, h)) * 2.0F - 1.0F;

	if (face == 0 && INPUT_HANDLER.keyPressed(KEY_N)) {
		debug = (debug + 1) % 3;
		logInfo("Debug: %d", debug);
	}
	pickedQuad = planet->getRayPickTerrainQuad(camera->getPickingRay(mouse));
	//
	//if (pickedQuad != NULL) {
	//	logInfo("%s",
	//		pickedQuad->getCubeFace() == X_NEG ? "X_NEG" :
	//		pickedQuad->getCubeFace() == X_POS ? "X_POS" :
	//		pickedQuad->getCubeFace() == Y_NEG ? "Y_NEG" :
	//		pickedQuad->getCubeFace() == Y_POS ? "Y_POS" :
	//		pickedQuad->getCubeFace() == Z_NEG ? "Z_NEG" :
	//		pickedQuad->getCubeFace() == Z_POS ? "Z_POS" : "UNKNOWN_FACE"
	//	);
	//
	//	//uint64 a = Time::now();
	//	//pickedQuad->updateNeighbours();
	//	//uint64 b = Time::now();
	//	//
	//	//logInfo("Took %f ms to update neighbours", (b - a) / 1000000.0);
	//}

	dmat4 faceTransformation = terrainQuad->getFaceTransformation();
	dmat4 localToScreen = camera->getViewProjectionMatrix() * planet->getViewerTransformation(camera->getPosition(true));
	dvec2 cameraFacePosition = planet->cubeFaceToLocalPoint(face, planet->worldToLocalPoint(camera->getPosition()));

	std::vector<TerrainInfo> instances = {};

	//if (false) {//terrainQuad->isLeaf()) {
		this->doRender(terrainQuad, 0, planet->getRadius() * Planet::scaleFactor, cameraFacePosition, localToScreen, faceTransformation, instances);
	//} else {
	//	TerrainRenderTask renderTask;
	//	renderTask.terrainQuad = terrainQuad;
	//	renderTask.planet = planet;
	//	renderTask.faceTransformation = faceTransformation;
	//	renderTask.cameraToScreen = cameraToScreen;
	//	renderTask.cameraFacePosition = cameraFacePosition;
	//
	//	logInfo("Adding render task to thread queue");
	//
	//	this->inputBuffers[0].push_back(renderTask);
	//	this->threadInput.notify_one();
	//	{
	//		logInfo("Waiting for processing result");
	//		std::unique_lock<std::mutex> lock(mut);
	//		this->threadOutput.wait(lock, [&] {
	//			return !this->outputBuffer[0].empty();
	//		});
	//
	//		logInfo("Received %d quads", this->outputBuffer[0].size());
	//	}
	//}

	uint64 t1 = Time::now();

	//logInfo("%f ms spent testing visibility", visibilietTestTime);

	if (!instances.empty()) {

		fvec3 viewerPosition = camera->getPosition();
		this->terrainProgram->useProgram(true);

		SCENE_GRAPH.applyUniforms(this->terrainProgram);
		planet->applyUniforms(this->terrainProgram);
		planet->tileSupplier->applyUniforms(this->terrainProgram);

		this->terrainProgram->setUniform("screenToLocal", inverse(localToScreen));
		this->terrainProgram->setUniform("localToScreen", localToScreen);
		this->terrainProgram->setUniform("faceTransformation", faceTransformation);
		this->terrainProgram->setUniform("viewerTransformation", planet->getViewerTransformation(viewerPosition));
		this->terrainProgram->setUniform("debugInt", debug);

		this->terrainInstanceBuffer->uploadInstanceData(0, sizeof(TerrainInfo) * instances.size(), static_cast<void*>(&instances[0]));

		// logInfo("%d instances", instances.size());
		this->terrainMesh->draw(instances.size(), 0, 0, this->terrainInstanceBuffer);

		this->terrainProgram->useProgram(false);
	}
	uint64 t2 = Time::now();

	//double renderTime = (t2 - t0) / 1000000.0;
	//double instanceTime = (t1 - t0) / 1000000.0;
	//logInfo("(FPS = %f) Took %f ms to render %d terrain tiles. %f ms to setup instances, %f ms for visibility, %f ms for setup", 1.0 / dt, renderTime, instances.size(), instanceTime);
}

void TerrainRenderer::doRender(TerrainQuad* terrainQuad, int depth, double r, dvec2 cameraFacePosition, dmat4 localToScreen, dmat4 faceTransformation, std::vector<TerrainInfo>& instances) {

	if (terrainQuad != NULL) {
		Frustum bb = terrainQuad->getDeformedBoundingBox();

		const bool visible = terrainQuad->getPlanet()->getVisibility(terrainQuad->getCubeFace(), &bb, terrainQuad->isLeaf());

		if (visible) {
			if (terrainQuad->isLeaf()) {
				TerrainInfo info = TerrainInfo();

				const dmat4 normals = terrainQuad->getWorldNormals();
				dmat4 corners = terrainQuad->getWorldCorners();
				corners[0] *= dvec4(dvec3(Planet::scaleFactor), 1.0);
				corners[1] *= dvec4(dvec3(Planet::scaleFactor), 1.0);
				corners[2] *= dvec4(dvec3(Planet::scaleFactor), 1.0);
				corners[3] *= dvec4(dvec3(Planet::scaleFactor), 1.0);

				fvec2 tilePosition = fvec2(0.0, 0.0);
				fvec2 tileSize = fvec2(1.0, 1.0);
				TileData* tileData = terrainQuad->getTileData(&tilePosition, &tileSize);

				//info.position = terrainQuad->getFacePosition();
				//info.size = terrainQuad->getSize();
				info.debug = fvec3(0.0, 0.0, 0.0);
				info.textureIndex = 0;
				info.neighbourDivisions = ivec4(-1);
				info.textureCoords = fvec4(0.0);

				for (int i = 0; i < 4; i++) {
					TerrainQuad* neighbourQuad = terrainQuad->getNeighbour((NeighbourIndex)i);
					if (neighbourQuad != NULL) {
						info.neighbourDivisions[i] = neighbourQuad->getDepth() - terrainQuad->getDepth();
						// -1 if neighbour is lower detail, +1 if neighbour is higher detail.
					}
				}

				//if (pickedQuad != NULL) {
				//	if (terrainQuad == pickedQuad) {
				//		info.debug = fvec3(1, 1, 1);
				//	} else if (terrainQuad == pickedQuad->getNeighbour(LEFT)) {
				//		info.debug = fvec3(0, 0, 1);
				//	} else if (terrainQuad == pickedQuad->getNeighbour(TOP)) {
				//		info.debug = fvec3(0, 1, 0);
				//	} else if (terrainQuad == pickedQuad->getNeighbour(RIGHT)) {
				//		info.debug = fvec3(0, 1, 1);
				//	} else if (terrainQuad == pickedQuad->getNeighbour(BOTTOM)) {
				//		info.debug = fvec3(1, 0, 0);
				//	}
				//}

				if (tileData != NULL) {
					double tileTimeCreated = (Time::now() - tileData->getTimeCreated()) / 1000000000.0;
					double tileTimeGenerated = (Time::now() - tileData->getTimeGenerated()) / 1000000000.0;
					double tileTimeReadback = (Time::now() - tileData->getTimeReadback()) / 1000000000.0;
					
					info.debug = fvec3(
						1.0 / (1.0 + tileTimeCreated),
						1.0 / (1.0 + tileTimeGenerated),
						1.0 / (1.0 + tileTimeReadback)
					);
				
					info.textureIndex = tileData->getTextureIndex();
					info.textureCoords = fvec4(tilePosition, tileSize);
				}

				// set terrain quad tile active when in frustum.
				// upload tile uniforms
				//terrainQuad->getPlanet()->tileStorage->getTile(terrainQuad);

				info.quadCorners = localToScreen * corners;
				info.quadNormals = localToScreen * normals;

				instances.push_back(info);
			} else {
				QuadIndex order[4] = { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };

				terrainQuad->getNearFarOrdering(cameraFacePosition, order);

				this->doRender(terrainQuad->getChild(order[0]), depth + 1, r, cameraFacePosition, localToScreen, faceTransformation, instances);
				this->doRender(terrainQuad->getChild(order[1]), depth + 1, r, cameraFacePosition, localToScreen, faceTransformation, instances);
				this->doRender(terrainQuad->getChild(order[2]), depth + 1, r, cameraFacePosition, localToScreen, faceTransformation, instances);
				this->doRender(terrainQuad->getChild(order[3]), depth + 1, r, cameraFacePosition, localToScreen, faceTransformation, instances);
			}
		}
	}
}

void TerrainRenderer::threadProc(int32 id) {

	do {
		logInfo("Thread waiting for terrain quads to process.");
		std::unique_lock<std::mutex> lock(mut);
		threadInput.wait(lock, [&] {
			return !this->inputBuffers[id].empty();
		});

		if (!this->inputBuffers[id].empty()) {
			TerrainRenderTask task = this->inputBuffers[id].front();
			this->inputBuffers[id].pop_front();

			logInfo("Received terrain quad");

			std::vector<TerrainInfo> instances = {};
			this->doRender(task.terrainQuad, 0, task.planet->getRadius() * Planet::scaleFactor, task.cameraFacePosition, task.cameraToScreen, task.faceTransformation, instances);

			logInfo("Thread created %d terrain instances", instances.size());

			this->outputBuffer[id] = instances;
		}

		lock.unlock();
		this->threadOutput.notify_all();
	} while (true);
}


MeshData* TerrainRenderer::createTerrainTileMesh() {
	return MeshHelper::createPlane(fvec2(0.0F), fvec2(1.0F), ivec2(this->terrainResolution), mat3(1.0F), true, NULL, TERRAIN_VERTEX_LAYOUT);
}
