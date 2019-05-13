#include "FunctionComponent.h"
#include "core/engine/scene/SceneGraph.h"


FunctionComponent::FunctionComponent(RenderCallback* renderCallback, UpdateCallback* updateCallback):
	renderCallback(renderCallback), updateCallback(updateCallback) {}

FunctionComponent::~FunctionComponent() {}

void FunctionComponent::render(SceneGraph* sceneGraph, double partialTicks, double dt) {
	if (renderCallback != NULL) {
		(*renderCallback)(sceneGraph, parent, partialTicks, dt);
	}
}

void FunctionComponent::update(SceneGraph* sceneGraph, double dt) {
	if (updateCallback != NULL) {
		(*updateCallback)(sceneGraph, parent, dt);
	}
}

void FunctionComponent::setRenderCallback(RenderCallback* renderCallback) {
	this->renderCallback = renderCallback;
}

void FunctionComponent::setUpdateCallback(UpdateCallback* updateCallback) {
	this->updateCallback = updateCallback;
}
