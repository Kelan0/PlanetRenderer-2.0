#pragma once

#include "core/Core.h"

class GameObject;
class SceneGraph;

class GameComponent {
	friend class GameObject;

protected:
	GameObject* parent;

public:
	GameComponent(): 
		parent(NULL) {};

	virtual ~GameComponent() {}

	virtual void render(SceneGraph* sceneGraph, double partialTicks, double dt) = 0;

	virtual void update(SceneGraph* sceneGraph, double dt) = 0;
};

