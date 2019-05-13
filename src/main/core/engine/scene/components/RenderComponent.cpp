#include "RenderComponent.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/scene/GameObject.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/ShaderProgram.h"
#include <GL/glew.h>

RenderComponent::RenderComponent(GLMesh* mesh, ShaderProgram* program) {
	this->mesh = mesh;
	this->program = program;

	this->setFaceCullingMode(GL_NONE);

	this->ownsMesh = false;
	this->ownsProgram = false;
}

RenderComponent::RenderComponent(MeshData* mesh, ShaderProgram* program) {
	this->mesh = new GLMesh(mesh, mesh->getVertexLayout());
	this->program = program;

	this->setFaceCullingMode(GL_NONE);

	this->ownsMesh = true;
	this->ownsProgram = false;
}

RenderComponent::~RenderComponent() {
	if (this->ownsMesh) {
		delete this->mesh;
	}

	if (this->ownsProgram) {
		delete this->program;
	}
}

void RenderComponent::render(SceneGraph* sceneGraph, double partialTicks, double dt) {
	this->program->useProgram(true);

	if (this->cullMode == GL_BACK || this->cullMode == GL_FRONT) {
		glEnable(GL_CULL_FACE);
		glCullFace(this->cullMode);
	} else {
		glDisable(GL_CULL_FACE);
	}

	sceneGraph->applyUniforms(this->program);
	this->mesh->draw();

	this->program->useProgram(false);
}

void RenderComponent::update(SceneGraph* sceneGraph, double dt) {}

bool RenderComponent::setFaceCullingMode(int32 faceCullingMode) {
	if (faceCullingMode == GL_FRONT || faceCullingMode == GL_BACK || faceCullingMode == GL_NONE) {
		this->cullMode = faceCullingMode;
		return true;
	}

	return false;
}

int32 RenderComponent::getFaceCullingMode() const {
	return this->cullMode;
}
