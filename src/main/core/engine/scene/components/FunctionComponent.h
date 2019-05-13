#pragma once

#include "core/Core.h"
#include "core/engine/scene/GameComponent.h"

class SceneGraph;

class FunctionComponent : public GameComponent {
public:
	using RenderCallback = typename std::function<void(SceneGraph*, GameObject*, double, double)>;
	using UpdateCallback = typename std::function<void(SceneGraph*, GameObject*, double)>;

private:
	RenderCallback* renderCallback;
	UpdateCallback* updateCallback;

public:
	FunctionComponent(RenderCallback* renderCallback = NULL, UpdateCallback* updateCallback = NULL);

	~FunctionComponent();

	void render(SceneGraph* sceneGraph, double partialTicks, double dt) override;

	void update(SceneGraph* sceneGraph, double dt) override;

	void setRenderCallback(RenderCallback* renderCallback);

	void setUpdateCallback(UpdateCallback* updateCallback);
};

