#pragma once

#include "core/Core.h"

class Atmosphere;
class FrameBuffer;
class ShaderProgram;
class GLMesh;
class DeferredRenderer;
class HistogramRenderer;
class ScreenRenderer;

class AtmosphereRenderer
{
private:
	ScreenRenderer* screenRenderer;
	ShaderProgram* atmosphereProgram;
	FrameBuffer* atmosphereFrameBuffer;

	uint32 screenTexture; // The final RGB screen texture after the atmosphere rendering pass.

	uvec2 screenResolution;

	std::vector<Atmosphere*> atmospheres;

public:
	AtmosphereRenderer(ScreenRenderer* screenRenderer);
	~AtmosphereRenderer();

	void initializeScreenResolution(uvec2 resolution);

	bool render(double partialTicks, double dt);

	void addAtmosphere(Atmosphere* atmosphere);

	uvec2 getScreenResolution() const;

	uint32 getScreenTexture() const;
};

