#pragma once

#include "core/Core.h"
#include "core/engine/scene/Transformation.h"

class GameObject;
class GameComponent;
class SceneGraph;

class GameObject
{
private:
	Transformation transformation;
	GameObject* parent;
	std::string name;

	int sceneTreeDepth;

	std::map<std::string, GameObject*> children;
	std::map<std::string, GameComponent*> components;

	void setParent(GameObject* parent, std::string id);

	void updateSceneTree();

	bool isValidId(std::string id);

public:
	GameObject(Transformation transformation = Transformation());

	GameObject(GameObject* parent, std::string name, Transformation transformation = Transformation());

	~GameObject();

	virtual void render(SceneGraph* sceneGraph, double partialTicks, double dt);

	virtual void update(SceneGraph* sceneGraph, double dt);

	std::string getName();

	std::vector<std::string> getChildNames();

	int32 getChildCount();

	GameObject* getParent();

	GameObject* getRoot();

	GameObject* findClosestAncestor(std::string id);

	GameObject* findClosestDescendent(std::string id);

	int32 getSceneTreeDepth();

	bool hasChild(std::string id);

	GameObject* getChild(std::string id);

	GameObject* addChild(std::string id, GameObject* object);

	GameObject* removeChild(std::string id);

	bool removeChild(GameObject* object);

	bool hasComponent(std::string id);

	GameComponent* getComponent(std::string id);

	GameComponent* addComponent(std::string id, GameComponent* component);

	GameComponent* removeComponent(std::string id);

	bool removeComponent(GameComponent* component);

	void setTransformation(Transformation transformation);

	Transformation& getTransformation();

	Transformation getGlobalTransformation();
};

