#pragma once

#include "core/Core.h"

class FrameBuffer;
class ShaderProgram;
class GLMesh;
class DeferredRenderer;
class HistogramRenderer;

class ScreenRenderer {
private:
	GLMesh* screenQuad;

	ShaderProgram* screenShader;

	FrameBuffer* screenBuffer;


	// Array of atmosphere renderers whicha re added to when a different atmosphere renderer is called, and cleared each frame. Atm should be a post process effect, after the deferred stage.

	DeferredRenderer* deferredRenderer;
	HistogramRenderer* histogramRenderer;

	uint32 albedoTexture; // red, green, blue
	uint32 glowTexture; // red, green, blue
	uint32 normalTexture; // x, y, z
	uint32 positionTexture; // x, y, z
	uint32 specularEmissionTexture; // specular, emission
	uint32 depthTexture; // depth32

	uint32 screenTexture;

	uint32 msaaSamples;
	bool fixedSampleLocations;

	uvec2 screenResolution;

	float eyeAdaptationRateIncr; // The adaptation rate for increasing scene brightness
	float eyeAdaptationRateDecr; // The adaptation rate for decreasing scene brightness
	float currScreenExposure; // The exposure of the screen in the current frame.


	void renderBloom();
public:
	ScreenRenderer();
	~ScreenRenderer();

	void init();

	void render(double partialTicks, double dt);

	void applyUniforms(ShaderProgram* program);

	void bindScreenBuffer() const;

	bool setResolution(uvec2 resolution);

	uvec2 getResolution() const;

	uint32 getMSAASamples() const;

	uint32 getAlbedoTexture();

	uint32 getGlowTexture();

	uint32 getNormalTexture();

	uint32 getPositionTexture();

	uint32 getSpecularEmissionTexture();
	
	uint32 getDepthTexture();

	uint32 getScreenTexture();

	GLMesh* getScreenQuad();

	DeferredRenderer* getDeferredRenderer() const;

	HistogramRenderer* getHistogramRenderer() const;
};

