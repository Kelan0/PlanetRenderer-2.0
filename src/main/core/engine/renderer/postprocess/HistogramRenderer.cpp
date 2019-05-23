#include "HistogramRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"
#include "core/engine/renderer/ScreenRenderer.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/util/Time.h"

#include <GL/glew.h>


HistogramRenderer::HistogramRenderer(ScreenRenderer* screenRenderer):
	screenRenderer(screenRenderer) {
	this->binCount = 256;
	this->histogramDownsample = 4;
	this->brightnessRange = 4.0; // x brighter than a pixel.
	this->transferBufferCount = 1;
	this->transferSync = new GLsync[this->transferBufferCount]();

	this->histogramShader = new ShaderProgram();
	this->histogramShader->addShader(GL_VERTEX_SHADER, "histogram/vert.glsl");
	this->histogramShader->addShader(GL_FRAGMENT_SHADER, "histogram/frag.glsl");
	this->histogramShader->addAttribute(0, "vs_vertexPosition");
	this->histogramShader->completeProgram();

	this->setBinCount(256.0);
}


HistogramRenderer::~HistogramRenderer()
{
}

bool HistogramRenderer::render(double partialTicks, double dt) {

	this->histogramFrameBuffer->bind(this->binCount, 1);
	glEnable(GL_TEXTURE_2D);

	glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->histogramShader->useProgram(true);
	this->histogramShader->setUniform("textureSampler", 0);
	this->histogramShader->setUniform("textureSize", fvec2(this->screenResolution));
	this->histogramShader->setUniform("histogramSize", fvec2(this->histogramResolution));
	this->histogramShader->setUniform("histogramBinCount", int32(this->binCount));
	this->histogramShader->setUniform("histogramBrightnessRange", int32(this->brightnessRange));

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->screenRenderer->getScreenTexture());

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (int i = 0; i < 4; i++) {
		this->histogramShader->setUniform("channel", i);
		this->samplePointMesh->draw();
	}
	glDisable(GL_BLEND);


	// Histogram pixel readback request

	for (int i = 0; i < this->transferBufferCount; i++) {
		uint32 readSize = this->binCount * sizeof(fvec4);
		uint32 readOffset = i * readSize;

		if (this->transferSync[i] == NULL) {
			// Make a readback request in the first available buffer.
			glBindBuffer(GL_PIXEL_PACK_BUFFER, this->transferBuffer);
			glBindTexture(GL_TEXTURE_2D, this->graphTexture);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (void*)readOffset);

			this->transferSync[i] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}
		else {
			// Check if any readbacks have been signalled.
			int32 result;
			glGetSynciv(this->transferSync[i], GL_SYNC_STATUS, sizeof(result), NULL, &result);

			if (result == GL_SIGNALED) {
				// Read the data back and copy it.
				glBindBuffer(GL_PIXEL_PACK_BUFFER, this->transferBuffer);
				vec4* data = static_cast<vec4*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, readOffset, readSize, GL_MAP_READ_BIT));
				std::memcpy(this->graphData, data, readSize);

				glDeleteSync(this->transferSync[i]);
				this->transferSync[i] = NULL;

				glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

				// Build the normalized cumulative distribution function
				for (int i = 0; i < this->binCount; i++) {
					// Normalized CDF
					fvec4 data = this->graphData[i] / (float)(this->histogramResolution.x * this->histogramResolution.y);

					if (i == 0) {
						this->cumulativeData[i] = data;
					}
					else {
						this->cumulativeData[i] = data + this->cumulativeData[i - 1];
					}
				}

				// Update CDF texture GPU-side
				glBindTexture(GL_TEXTURE_2D, this->cumulativeTexture);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->binCount, 1, GL_RGBA, GL_FLOAT, (void*)this->cumulativeData);

				// Calculate average screen luminance and exposure
				float k = 0.69314718055994530941723212145818;
				float lowPercent = 0.62;
				float highPercent = 0.94;
				float logMin = -14.0;
				float logMax = 13.0;
				float scale = 1.0 / (logMax - logMin);
				float offset = -logMin * scale;
				float minLum = exp(-5.0 * k);
				float maxLum = exp(+5.0 * k);

				float maxHistogramValue = 0.0;
				for (int i = 0; i < this->binCount; i++) {
					maxHistogramValue = glm::max(maxHistogramValue, this->graphData[i].a);
				}
				float invMaxHistogramValue = 1.0F / maxHistogramValue;

				float sumLuminance = 0.0;
				for (int i = 0; i < this->binCount; i++) {
					sumLuminance += this->graphData[i].a * invMaxHistogramValue;
				}

				fvec4 filter = fvec4(0.0, 0.0, sumLuminance * lowPercent, sumLuminance * highPercent);

				for (int i = 0; i < this->binCount; i++) {
					float t = (float)i / (float)this->binCount;
					float binVal = this->graphData[i].a * invMaxHistogramValue;
					float offset = glm::min(filter.z, binVal);

					binVal -= offset;
					filter.z -= offset;
					filter.w -= offset;
					binVal = glm::min(filter.w, binVal);
					filter.w -= binVal;

					float binLum = exp2((t - offset) / scale);
					filter.x += binLum * binVal;
					filter.y += binVal;
				}

				float avgLuminance = glm::clamp(filter.x / glm::max(filter.y, 1e-4F), minLum, maxLum);

				float keyValue = 1.03 - (2.0 / (2.0 + glm::log2(avgLuminance + 1.0)));
				this->expectedScreenExposure = glm::clamp((keyValue / avgLuminance) / 0.09, 0.25, 4.0);
				//logInfo("Average luminance = %f, exposure = %f, scale = %f", avgLuminance, exposure);
			}
		}
	}

	return true;
}

void HistogramRenderer::applyUniforms(ShaderProgram* program) {
	program->setUniform("histogramBinCount", int32(this->binCount));
	program->setUniform("histogramDownsample", int32(this->histogramDownsample));
	program->setUniform("histogramResolution", fvec2(this->histogramResolution));
}

void HistogramRenderer::initializeScreenResolution(uvec2 screenResolution) {
	if (this->screenResolution != screenResolution) {
		this->screenResolution = screenResolution;
		this->histogramResolution = uvec2(screenResolution / this->histogramDownsample);
		
		VertexLayout histogramPointAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

		int32 vertexCount = this->histogramResolution.x * this->histogramResolution.y;

		MeshData* histogramPointMesh = new MeshData(vertexCount, vertexCount, histogramPointAttributes);

		fvec3 vertex = fvec3(0.0, 0.0, 0.0);
		for (int i = 0; i < vertexCount; i++) {
			vertex.x = (float)(i / this->histogramResolution.y) / (float)this->histogramResolution.x;
			vertex.y = (float)(i % this->histogramResolution.y) / (float)this->histogramResolution.y;
			// vertices.push_back(Vertex(vertex));
			// indices.push_back(i);
			histogramPointMesh->addIndex(histogramPointMesh->addVertex(vertex));
		}

		this->samplePointMesh = new GLMesh(histogramPointMesh, histogramPointAttributes);
		this->samplePointMesh->setPrimitive(GL_POINTS);
	}
}

void HistogramRenderer::setBinCount(uint32 binCount) {
	if (binCount < 5) {
		logError("Cannot initialize histogram with too few bins");
		return;
	}

	this->binCount = binCount;

	delete this->histogramFrameBuffer;

	glEnable(GL_TEXTURE_2D);

	glDeleteTextures(1, &this->graphTexture);
	glGenTextures(1, &this->graphTexture);

	glDeleteTextures(1, &this->cumulativeTexture);
	glGenTextures(1, &this->cumulativeTexture);

	glBindTexture(GL_TEXTURE_2D, this->graphTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->binCount, 1, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, this->cumulativeTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->binCount, 1, 0, GL_RGBA, GL_FLOAT, NULL);

	//uint32* drawBuffers = new uint32[1]{ GL_COLOR_ATTACHMENT0 };

	this->histogramFrameBuffer = new FrameBuffer();
	this->histogramFrameBuffer->bind(this->binCount, 1);
	//this->histogramBuffer->setDrawBuffers(1, drawBuffers);
	//delete[] drawBuffers;

	this->histogramFrameBuffer->createColourTextureAttachment(0, this->graphTexture, GL_TEXTURE_2D);
	logInfo("Validating histogram framebuffer");
	this->histogramFrameBuffer->checkStatus(true);


	glGenBuffers(1, &this->transferBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->transferBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, this->transferBufferCount * this->binCount * sizeof(fvec4), NULL, GL_STREAM_COPY);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	delete[] this->graphData;
	delete[] this->cumulativeData;
	this->graphData = new fvec4[this->binCount]();
	this->cumulativeData = new fvec4[this->binCount]();
}

uint32 HistogramRenderer::getGraphTexture() const {
	return this->graphTexture;
}

uint32 HistogramRenderer::getCumulativeTexture() const {
	return this->cumulativeTexture;
}

uvec2 HistogramRenderer::getScreenResolution() const {
	return this->screenResolution;
}

uvec2 HistogramRenderer::getHistogramResolution() const {
	return this->histogramResolution;
}

uint32 HistogramRenderer::getDownsample() const {
	return this->histogramDownsample;
}

float HistogramRenderer::getBrightnessRange() const {
	return this->brightnessRange;
}

float HistogramRenderer::getExpectedScreenExposure() const {
	return this->expectedScreenExposure;
}
