#pragma once

#include "core/Core.h"
#include "core/engine/scene/GameComponent.h"

class GLMesh;
class MeshData;
class ShaderProgram;

class RenderComponent : public GameComponent {
private:
	GLMesh* mesh;
	ShaderProgram* program;

	int32 cullMode;

	bool ownsMesh;
	bool ownsProgram;

public:
	RenderComponent(GLMesh* mesh, ShaderProgram* program);

	RenderComponent(MeshData* mesh, ShaderProgram* program);

	~RenderComponent();

	void render(SceneGraph* sceneGraph, double partialTicks, double dt) override;

	void update(SceneGraph* sceneGraph, double dt) override;

	bool setFaceCullingMode(int32 cullMode);

	int32 getFaceCullingMode() const;
};

