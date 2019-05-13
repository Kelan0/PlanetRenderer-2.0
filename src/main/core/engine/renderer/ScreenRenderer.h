#pragma once

#include "core/Core.h"

class FrameBuffer;
class ShaderProgram;
class GLMesh;

class ScreenRenderer {
private:
	GLMesh* screenQuad;

	ShaderProgram* screenShader;
	FrameBuffer* screenBuffer;

	uint32 albedoTexture; // red, green, blue
	uint32 normalTexture; // x, y, z
	uint32 specularEmissionTexture; // specular, emission
	uint32 depthTexture; // depth32

	uint32 msaaSamples;
	bool fixedSampleLocations;

public:
	ScreenRenderer();
	~ScreenRenderer();

	void init(int windowWidth, int windowHeight);

	void updateTexturesMultisample(int32 width, int32 height);
};

