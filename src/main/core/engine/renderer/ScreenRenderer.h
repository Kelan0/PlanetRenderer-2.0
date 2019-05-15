#pragma once

#include "core/Core.h"

class FrameBuffer;
class ShaderProgram;
class GLMesh;

class ScreenRenderer {
private:
	GLMesh* screenQuad;
	GLMesh* histogramPoints;

	ShaderProgram* screenShader;
	ShaderProgram* histogramShader;

	FrameBuffer* screenBuffer;
	FrameBuffer* histogramBuffer;

	uint32 histogramTexture;

	uint32 albedoTexture; // red, green, blue
	uint32 normalTexture; // x, y, z
	uint32 specularEmissionTexture; // specular, emission
	uint32 depthTexture; // depth32

	uint32 msaaSamples;
	bool fixedSampleLocations;

	uvec2 resolution;
	uvec2 histogramResolution;

	uint32 histogramBinCount;

public:
	ScreenRenderer();
	~ScreenRenderer();

	void init();

	void render(double partialTicks, double dt);

	void bindScreenBuffer() const;

	bool setHistogram(uint32 binCount);

	bool setResolution(uvec2 resolution);

	uvec2 getResolution() const;
};

