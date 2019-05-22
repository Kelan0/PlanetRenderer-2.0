#pragma once

#include "core/Core.h"

class FrameBuffer;
class ShaderProgram;
class GLMesh;
class ScreenRenderer;

class DeferredRenderer
{
private:
	ScreenRenderer* screenRenderer;
	ShaderProgram* deferredShader;
	FrameBuffer* deferredFrameBuffer;

	uint32 screenTexture; // The final RGB screen texture after deferred rendering pass.

	uvec2 screenResolution;
public:
	DeferredRenderer(ScreenRenderer* screenRenderer);

	~DeferredRenderer();

	void initializeScreenResolution(uvec2 screenResolution);

	void render(double partialTicks, double dt);

	uvec2 getScreenResolution() const;

	uint32 getScreenTexture() const;
};

