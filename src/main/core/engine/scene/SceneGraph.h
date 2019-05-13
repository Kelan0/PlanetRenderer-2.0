#pragma once

#include "core/Core.h"
#include "core/engine/scene/Transformation.h"
#include "core/engine/scene/GameObject.h"
#include "core/engine/scene/GameComponent.h"

class ShaderProgram;
class Camera;
struct Vertex;

typedef enum WireframeMode {
	SHOW_FACES = 0,
	SHOW_WIRES = 1,
	SHOW_BOTH = 2
} WireframeMode;

class SceneGraph {
private:
	std::vector<mat4> transformationStack;
	mat4 currModelMatrix;

	bool enableLighting;

	GameObject* root;
	Camera* camera;

	WireframeMode wireframeMode;

	ShaderProgram* debugShader;

public:
	SceneGraph();
	~SceneGraph();

	void init();

	void render(double partialTicks, double dt);

	void update(double dt);

	void renderDebug(uint32 mode, uint32 count, Vertex vertices[], int indices[], dmat4 modelMatrix = dmat4(1.0));

	void pushTransformationState(Transformation transformation);

	void popTransformationState();

	Transformation getTransformationState();

	GameObject* getRoot();

	Camera* getCamera();

	void applyUniforms(ShaderProgram* program);

	WireframeMode getWireframeMode() const;

	void setWireframeMode(WireframeMode wireframeMode);

	void toggleWireframeMode();

	bool isLightingEnabled() const;

	void setLightingEnabled(bool enabled);
};

