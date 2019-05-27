#include "SceneGraph.h"
#include "core/application/Application.h"
#include "core/event/EventHandler.h"
#include "core/engine/scene/GameObject.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/geometry/MeshData.h"
#include "core/engine/terrain/Planet.h"
#include "core/engine/renderer/Camera.h"

#include <GL/glew.h>

SceneGraph::SceneGraph() {
	this->currModelMatrix = mat4(1.0F);

	this->root = new GameObject();
	this->camera = new Camera();

	this->enableLighting = true;
}

SceneGraph::~SceneGraph() {
	delete this->root;
}

void SceneGraph::init() {
	EVENT_HANDLER.subscribe(EventLambda(WindowResizeEvent) {
		// Assuming camera is never null...
		camera->setAspectRatio(Application::getWindowAspectRatio());
	});
}

void SceneGraph::render(double partialTicks, double dt) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (this->wireframeMode == SHOW_FACES) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	} else if (this->wireframeMode == SHOW_WIRES) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	this->camera->render(partialTicks, dt);
	this->root->render(this, partialTicks, dt);
}

void SceneGraph::update(double dt) {
	this->root->update(this, dt);
}

//void SceneGraph::renderDebug(uint32 mode, uint32 count, Vertex vertices[], int indices[], dmat4 matrix) {
//	bool lightingEnabled = this->isLightingEnabled();
//	if (mode == GL_LINES) {
//		this->setLightingEnabled(false);
//		this->setLightingEnabled(lightingEnabled);
//	}
//
//	this->debugShader->useProgram(true);
//	this->applyUniforms(this->debugShader);
//
//	dvec3 cp = this->camera->getPosition(true);
//
//	this->debugShader->setUniform("modelMatrix", dmat4(1.0));
//
//	glBegin(mode);
//
//	for (int i = 0; i < count; i++) {
//		Vertex v = matrix * vertices[indices[i]];
//		glVertexAttrib2f(2, v.texture.x, v.texture.y);
//		glVertexAttrib3f(1, v.normal.x, v.normal.y, v.normal.z);
//		glVertexAttrib3f(0, 
//			(v.position.x - cp.x) * Planet::scaleFactor,
//			(v.position.y - cp.y) * Planet::scaleFactor,
//			(v.position.z - cp.z) * Planet::scaleFactor
//		);
//	}
//
//	glEnd();
//
//	this->debugShader->useProgram(false);
//	if (mode == GL_LINES) {
//		this->setLightingEnabled(lightingEnabled);
//	}
//}

void SceneGraph::pushTransformationState(Transformation transformation) {
	this->transformationStack.push_back(this->currModelMatrix);
	this->currModelMatrix = transformation.getMatrix() * this->currModelMatrix;
}

void SceneGraph::popTransformationState() {
	this->currModelMatrix = this->transformationStack.back();
	this->transformationStack.pop_back();
}

Transformation SceneGraph::getTransformationState() {
	return Transformation(this->currModelMatrix);
}

GameObject* SceneGraph::getRoot() {
	return this->root;
}

Camera* SceneGraph::getCamera() {
	return this->camera;
}

void SceneGraph::applyUniforms(ShaderProgram* program) {
	if (program != NULL) {
		program->setUniform("modelMatrix", this->currModelMatrix);
		program->setUniform("normalMatrix", mat4(1.0F));
		program->setUniform("scaleFactor", float(Planet::scaleFactor));
		program->setUniform("enableLighting", this->enableLighting);
		this->camera->applyUniforms(program);
	}
}

WireframeMode SceneGraph::getWireframeMode() const {
	return this->wireframeMode;
}

void SceneGraph::setWireframeMode(WireframeMode wireframeMode) {
	this->wireframeMode = wireframeMode;
}

void SceneGraph::toggleWireframeMode() {
	if (this->wireframeMode == SHOW_FACES) {
		this->wireframeMode = SHOW_WIRES;
	} else if (this->wireframeMode == SHOW_WIRES) {
		this->wireframeMode = SHOW_BOTH;
	} else {
		this->wireframeMode = SHOW_FACES;
	}
}

bool SceneGraph::isLightingEnabled() const {
	return this->enableLighting;
}

void SceneGraph::setLightingEnabled(bool enabled) {
	this->enableLighting = enabled;
}
