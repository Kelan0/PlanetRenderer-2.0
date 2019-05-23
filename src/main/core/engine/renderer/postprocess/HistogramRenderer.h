#pragma once

#include "core/Core.h"

class FrameBuffer;
class ShaderProgram;
class GLMesh;
class ScreenRenderer;

typedef struct __GLsync* GLsync;

class HistogramRenderer
{
private:
	ScreenRenderer* screenRenderer;

	GLMesh* samplePointMesh;
	ShaderProgram* histogramShader;
	FrameBuffer* histogramFrameBuffer;

	uint32 transferBufferCount; // The number of histogram sync buffers.
	uint32 graphTexture; // The histogram texture.
	uint32 cumulativeTexture; // The cumulative distribution function texture.
	uint32 transferBuffer; // pixel buffer for asynchronous histoghram transfer from vram.
	GLsync* transferSync; // Array of sync objects for each buffer.
	vec4* graphData; // The histogram data read back from the VRAM. this may be a few frames behind.
	vec4* cumulativeData; // The histogram cumulative distribution calculated from the current histogramData array.

	uvec2 screenResolution;
	uvec2 histogramResolution;
	uint32 histogramDownsample;
	float brightnessRange;
	float expectedScreenExposure;

	uint32 binCount;

public:
	HistogramRenderer(ScreenRenderer* screenRenderer);
	~HistogramRenderer();

	bool render(double partialTicks, double dt);

	void applyUniforms(ShaderProgram* program);

	void initializeScreenResolution(uvec2 screenResolution);

	void setBinCount(uint32 binCount);

	uint32 getGraphTexture() const;

	uint32 getCumulativeTexture() const;

	uvec2 getScreenResolution() const;

	uvec2 getHistogramResolution() const;

	uint32 getDownsample() const;

	float getBrightnessRange() const;

	float getExpectedScreenExposure() const;
};

