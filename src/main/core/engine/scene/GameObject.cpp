#include "GameObject.h"
#include "core/application/Application.h"
#include "core/engine/scene/GameComponent.h"
#include "core/engine/scene/SceneGraph.h"


bool isValidId(std::string id);



GameObject::GameObject(Transformation transformation) {
	this->setTransformation(transformation);
	this->parent = NULL;
	this->sceneTreeDepth = 0;
}

GameObject::GameObject(GameObject* parent, std::string name, Transformation transformation) {
	this->setTransformation(transformation);
	this->parent = NULL;
	this->sceneTreeDepth = 0;

	if (parent != NULL) {
		parent->addChild(name, this);
	} else {
		this->parent = NULL;
		this->sceneTreeDepth = 0;
	}
}

GameObject::~GameObject() {
	if (this->parent != NULL) {
		this->parent->removeChild(this);
	}

	for (auto it = this->children.begin(); it != this->children.end(); it++) {
		delete it->second;
	}

	for (auto it = this->components.begin(); it != this->components.end(); it++) {
		delete it->second;
	}
}


void GameObject::setParent(GameObject* _parent, std::string _id) {
	if (this->parent != NULL) {
		if (!this->parent->removeChild(this)) {
			logWarn("New parent was set directly, but this GameObject was not found in its old parent. The parent was not notified of its removal.");
		}
	}

	// TODO: check for circular branches.

	this->parent = _parent;
	this->name = _id;
	this->updateSceneTree();
}

void GameObject::updateSceneTree() {
	if (this->parent != NULL) {
		this->sceneTreeDepth = this->parent->getSceneTreeDepth() + 1;
	} else {
		this->sceneTreeDepth = 0;
	}

	for (auto it = this->children.begin(); it != this->children.end(); it++) {
		if (it->second != NULL) {
			it->second->updateSceneTree();
		}
	}
}

bool GameObject::isValidId(std::string id) {
	return id.size() > 0;
}

void GameObject::render(SceneGraph* sceneGraph, double partialTicks, double dt) {
	sceneGraph->pushTransformationState(this->getTransformation());

	for (auto it = this->components.begin(); it != this->components.end(); it++) {
		if (it->second != NULL) {
			it->second->render(sceneGraph, partialTicks, dt);
		}
	}

	for (auto it = this->children.begin(); it != this->children.end(); it++) {
		if (it->second != NULL) {
			it->second->render(sceneGraph, partialTicks, dt);
		}
	}

	sceneGraph->popTransformationState();
}

void GameObject::update(SceneGraph* sceneGraph, double dt) {
	sceneGraph->pushTransformationState(this->getTransformation());

	for (auto it = this->components.begin(); it != this->components.end(); it++) {
		if (it->second != NULL) {
			it->second->update(sceneGraph, dt);
		}
	}

	for (auto it = this->children.begin(); it != this->children.end(); it++) {
		if (it->second != NULL) {
			it->second->update(sceneGraph, dt);
		}
	}

	sceneGraph->popTransformationState();
}

std::string GameObject::getName() {
	return this->name;
}

std::vector<std::string> GameObject::getChildNames() {
	std::vector<std::string> names;
	names.reserve(this->children.size());

	for (auto it = this->children.begin(); it != this->children.end(); it++) {
		names.push_back(it->first);
	}

	return names;
}

int32 GameObject::getChildCount() {
	return this->children.size();
}

GameObject* GameObject::getParent() {
	return this->parent;
}

GameObject* GameObject::getRoot() {
	if (this->parent != NULL) {
		return this->parent->getRoot();
	} else {
		return this;
	}
}

GameObject* GameObject::findClosestAncestor(std::string id) {
	if (this->parent != NULL) {
		std::vector<std::string> names = this->parent->getChildNames();
		for (auto it = names.begin(); it != names.end(); it++) {
			if (*it == id) {
				return this->parent->getChild(*it);
			}
		}
	}

	return NULL;
}

GameObject* GameObject::findClosestDescendent(std::string id) { // TODO: BFS/DFS option
	//or (auto it = children.begin(); it != children.end(); it++) {
	//	if (it->first == id) {
	//		return it->second;
	//	}
	//

	return NULL;
}

int32 GameObject::getSceneTreeDepth() {
	return this->sceneTreeDepth;
}

bool GameObject::hasChild(std::string id) {
	if (this->isValidId(id)) {
		auto it = this->children.find(id);
		return it != this->children.end() && it->second != NULL;
	}

	return false;
}

GameObject* GameObject::getChild(std::string id) {
	if (this->isValidId(id)) {
		auto it = this->children.find(id);

		if (it != this->children.end()) {
			return it->second;
		}
	}

	return NULL;
}

GameObject* GameObject::addChild(std::string id, GameObject* object) {
	GameObject* replaced = NULL;

	if (this->isValidId(id)) {
		auto it = this->children.find(id);

		if (it != this->children.end()) {
			replaced = it->second;
		}

		if (object == NULL) {
			this->children.erase(it);
		} else {
			this->children[id] = object;
			object->setParent(this, id);
		}
	}

	return replaced;
}

GameObject* GameObject::removeChild(std::string id) {
	auto it = this->children.find(id);

	GameObject* removed = NULL;

	if (it != this->children.end()) {
		removed = it->second;
		this->children.erase(it);

		if (removed != NULL) {
			removed->setParent(NULL, "");
		}
	}

	return removed;
}

bool GameObject::removeChild(GameObject* object) {
	for (auto it = this->children.begin(); it != this->children.end(); it++) {
		GameObject* removed = it->second;

		if (removed == object) {
			this->children.erase(it);

			if (removed != NULL) {
				removed->setParent(NULL, "");
			}
			return true;
		}
	}

	return false;
}

bool GameObject::hasComponent(std::string id) {
	if (this->isValidId(id)) {
		auto it = this->components.find(id);
		return it != this->components.end() && it->second != NULL;
	}

	return false;
}

GameComponent* GameObject::getComponent(std::string id) {
	if (this->isValidId(id)) {
		auto it = this->components.find(id);

		if (it != this->components.end()) {
			return it->second;
		}
	}

	return NULL;
}

GameComponent* GameObject::addComponent(std::string id, GameComponent* component) {
	GameComponent* replaced = NULL;

	if (this->isValidId(id)) {
		auto it = this->components.find(id);

		if (it != this->components.end()) {
			replaced = it->second;
		}

		if (component == NULL) {
			this->components.erase(it);
		} else {
			this->components[id] = component;
			component->parent = this;
		}
	}

	return replaced;
}

GameComponent* GameObject::removeComponent(std::string id) {
	auto it = this->components.find(id);

	GameComponent* removed = NULL;

	if (it != this->components.end()) {
		removed = it->second;
		this->components.erase(it);

		if (removed != NULL) {
			removed->parent = NULL;
		}
	}

	return removed;
}

bool GameObject::removeComponent(GameComponent* component) {
	for (auto it = this->components.begin(); it != this->components.end(); it++) {
		GameComponent* removed = it->second;
		if (removed == component) {
			this->components.erase(it);

			if (removed != NULL) {
				removed->parent = NULL;
			}

			return true;
		}
	}

	return false;
}

void GameObject::setTransformation(Transformation transformation) {
	this->transformation = transformation;
}

Transformation& GameObject::getTransformation() {
	return this->transformation;
}

Transformation GameObject::getGlobalTransformation() { // This is potentially slow to calculate for big scene trees... this should be cached when possible.
	Transformation transform(this->getTransformation());

	if (parent != NULL) {
		Transformation p = parent->getGlobalTransformation();

		transform.translate(p.getTranslation());
		transform.rotate(p.getOrientation());
		transform.scale(p.getScale());
	}

	return transform;
}