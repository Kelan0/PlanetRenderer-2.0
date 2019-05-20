#pragma once

#include "core/Core.h"

class FrameBuffer;
class ShaderProgram;
class GLMesh;

typedef struct __GLsync* GLsync;

class ScreenRenderer {
private:
	GLMesh* screenQuad;
	GLMesh* histogramPoints;

	ShaderProgram* screenShader;
	ShaderProgram* histogramShader;

	FrameBuffer* screenBuffer;
	FrameBuffer* histogramBuffer;

	uint32 histogramTransferBufferCount; // The number of histogram sync buffers.
	uint32 histogramTexture; // The histogram texture.
	uint32 histogramCumulativeTexture; // The cumulative distribution function texture.
	uint32 histogramTransferBuffer; // pixel buffer for asynchronous histoghram transfer from vram.
	GLsync* histogramTransferSync; // Array of sync objects for each buffer.
	vec4* histogramData; // The histogram data read back from the VRAM. this may be a few frames behind.
	vec4* histogramCumulativeDistribution; // The histogram cumulative distribution

	uint32 albedoTexture; // red, green, blue
	uint32 normalTexture; // x, y, z
	uint32 positionTexture; // x, y, z
	uint32 specularEmissionTexture; // specular, emission
	uint32 depthTexture; // depth32

	uint32 msaaSamples;
	bool fixedSampleLocations;

	uvec2 screenResolution;
	uvec2 histogramResolution;
	uint32 histogramDownsample;
	float histogramBrightnessRange;

	float eyeAdaptationRateIncr; // The adaptation rate for increasing scene brightness
	float eyeAdaptationRateDecr; // The adaptation rate for decreasing scene brightness
	float expectedScreenExposure; // The exposure that the screen should adapt to.
	float currScreenExposure; // The exposure of the screen in the current frame.

	uint32 histogramBinCount;

	void updateHistogram(double dt);
public:
	ScreenRenderer();
	~ScreenRenderer();

	void init();

	void render(double partialTicks, double dt);

	void applyUniforms(ShaderProgram* program);

	void bindScreenBuffer() const;

	bool setHistogram(uint32 binCount);

	bool setResolution(uvec2 resolution);

	uvec2 getResolution() const;

	uint32 getAlbedoTexture();

	uint32 getNormalTexture();

	uint32 getPositionTexture();

	uint32 getSpecularEmissionTexture();
	
	uint32 getDepthTexture();
};

