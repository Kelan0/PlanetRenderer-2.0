#include "DebugRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/Camera.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/terrain/Planet.h"
#include "core/engine/geometry/MeshData.h"
#include "core/engine/scene/SceneGraph.h"

#include <GL/glew.h>


DebugRenderer::DebugRenderer() {
	this->debugShader = NULL;
	this->pointMesh = NULL;
	this->lineMesh = NULL;
	this->triangleMesh = NULL;

	this->colour = fvec4(1.0F, 1.0F, 1.0F, 1.0F);

	this->lineThickness = 1.0;
	this->enableDepth = true;
	this->enableLighting = true;
	this->enableBlend = false;

	this->blendSrcFactor = GL_ONE;
	this->blendDstFactor = GL_ONE_MINUS_SRC_ALPHA;

	this->currMeshData = NULL;
	this->currentMode = TRIANGLES;
}


DebugRenderer::~DebugRenderer() {
}

void DebugRenderer::init() {
	this->pointMesh = new GLMesh();
	this->pointMesh->setPrimitive(GL_POINTS);
	
	this->lineMesh = new GLMesh();
	this->lineMesh->setPrimitive(GL_LINES);
	
	this->triangleMesh = new GLMesh();
	this->triangleMesh->setPrimitive(GL_TRIANGLES);
	
	this->debugShader = new ShaderProgram();
	this->debugShader->addShader(GL_VERTEX_SHADER, "default/vert.glsl");
	this->debugShader->addShader(GL_FRAGMENT_SHADER, "default/frag.glsl");
	this->debugShader->addAttribute(0, "position");
	this->debugShader->addAttribute(1, "normal");
	this->debugShader->addAttribute(2, "texture");
	this->debugShader->addAttribute(3, "colour");
	this->debugShader->completeProgram();
}

void DebugRenderer::setLineThickness(float thickness) {
	this->lineThickness = thickness;
}

void DebugRenderer::setDepthEnabled(bool enabled) {
	this->enableDepth = enabled;
}

void DebugRenderer::setLightingEnabled(bool enabled) {
	this->enableLighting = enabled;
}

void DebugRenderer::setBlendEnabled(bool enabled) {
	this->enableBlend = enabled;
}

void DebugRenderer::setBlend(uint32 sfactor, uint32 dfactor) {
	this->blendSrcFactor = sfactor;
	this->blendDstFactor = dfactor;
}

void DebugRenderer::begin(uint32 mode) {
	if (this->currMeshData == NULL) {
		this->currMeshData = new MeshData();
		this->setLineThickness(1.0F);
		this->setDepthEnabled(true);
		this->setLightingEnabled(true);
		this->setBlendEnabled(false);
	
		if (mode == POINTS || mode == LINES || mode == TRIANGLES) {
			this->currentMode = mode;
		} else {
			this->currentMode = TRIANGLES; //Default to triangles.
		}
	} else {
		logError("Debug mesh is currently in progress. Cannot begin a new mesh");
	}
}

void DebugRenderer::render(std::vector<Vertex> vertices, std::vector<int32> indices, dmat4 modelMatrix) {
	if (this->currMeshData != NULL) {
		int32 indexOffset = this->currMeshData->getVertexCount();

		//dvec3 cp = SCENE_GRAPH.getCamera()->getPosition(true);

		for (int i = 0; i < vertices.size(); i++) {
			Vertex v = modelMatrix * vertices[i];
			//v.position = fvec3((dvec3(v.position) - cp) * Planet::scaleFactor);
			//
			this->currMeshData->addVertex(v);
		}

		for (int i = 0; i < indices.size(); i++) {
			this->currMeshData->addIndex(indexOffset + indices[i]);
		}
	}
}

void DebugRenderer::finish() {
	if (this->currMeshData != NULL) {
		if (this->currMeshData->getVertexCount() > 0 && this->currMeshData->getIndexCount() > 0) {
			this->debugShader->useProgram(true);
			SCENE_GRAPH.applyUniforms(this->debugShader);

			if (this->enableBlend) {
				glEnable(GL_BLEND);
				glBlendFunc(this->blendSrcFactor, this->blendDstFactor);
			} else {
				glDisable(GL_BLEND);
			}

			if (this->enableDepth) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}

			glLineWidth(this->lineThickness);
			this->debugShader->setUniform("lightingEnabled", this->enableLighting);
			this->debugShader->setUniform("modelMatrix", fmat4(1.0));
			this->debugShader->setUniform("lineThickness", this->lineThickness);
			this->debugShader->setUniform("cameraPosition", fvec3(SCENE_GRAPH.getCamera()->getPosition(true)));

			if (this->currentMode == POINTS) {
				this->pointMesh->uploadMeshData(this->currMeshData);
				this->pointMesh->draw();
			}

			if (this->currentMode == LINES) {
				this->lineMesh->uploadMeshData(this->currMeshData);
				this->lineMesh->draw();
			}

			if (this->currentMode == TRIANGLES) {
				this->triangleMesh->uploadMeshData(this->currMeshData);
				this->triangleMesh->draw();
			}

			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
		}

		delete this->currMeshData;
		this->currMeshData = NULL;
	}
}
