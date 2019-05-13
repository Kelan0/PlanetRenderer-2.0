#pragma once

#include "core/Core.h"

class Container {
public:
	virtual ~Container() {};
};

template<typename T>
class ObjectContainer : public Container {
public:
	T value;

	ObjectContainer(T value) :
		value(value) {}

	template<typename V>
	T cast(V v) {
		return dynamic_cast<T>(v);
	}
};

class ContainerMap {
private:
	std::map<std::string, Container*> containerMap;
public:

	ContainerMap() {}

	template<typename T>
	Container* put(std::string id, T val) {
		auto it = this->containerMap.find(id);
		
		Container* existing = NULL;
		if (it != this->containerMap.end()) {
			existing = it->second;
		}

		this->containerMap[id] = new ObjectContainer<T>(val);

		return existing;
	}


	template<typename T>
	T get(std::string id, T defaultVal) {
		auto it = this->containerMap.find(id);

		if (it != this->containerMap.end()) {
			Container* container = it->second;
			ObjectContainer<T>* objContainer = NULL;

			if (container != NULL && (objContainer = dynamic_cast<ObjectContainer<T>*>(container)) != NULL) {
				return objContainer->value;
			}
		}

		return defaultVal;
	}
};