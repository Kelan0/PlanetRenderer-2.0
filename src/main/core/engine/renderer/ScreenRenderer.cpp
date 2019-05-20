#include "ScreenRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/terrain/Planet.h"
#include "core/util/Time.h"

#include <GL/glew.h>



ScreenRenderer::ScreenRenderer() {
	this->msaaSamples = 4;
	this->fixedSampleLocations = true;
	this->histogramBinCount = 256;
	this->histogramDownsample = 4;
	this->histogramBrightnessRange = 4.0; // x brighter than a pixel.
	this->eyeAdaptationRateIncr = 2.0;
	this->eyeAdaptationRateDecr = 2.0;
	this->histogramTransferBufferCount = 1;
	this->histogramTransferSync = new GLsync[this->histogramTransferBufferCount]();
}


ScreenRenderer::~ScreenRenderer() {

}

void ScreenRenderer::init() {
	this->screenShader = new ShaderProgram();
	this->screenShader->addShader(GL_VERTEX_SHADER, "screen/vert.glsl");
	this->screenShader->addShader(GL_FRAGMENT_SHADER, "screen/frag.glsl");
	this->screenShader->addAttribute(0, "vs_vertexPosition");
	this->screenShader->completeProgram();

	this->histogramShader = new ShaderProgram();
	this->histogramShader->addShader(GL_VERTEX_SHADER, "histogram/vert.glsl");
	this->histogramShader->addShader(GL_FRAGMENT_SHADER, "histogram/frag.glsl");
	this->histogramShader->addAttribute(0, "vs_vertexPosition");
	this->histogramShader->completeProgram();

	VertexLayout screenAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

	MeshData* screenMesh = new MeshData(4, 6, screenAttributes);
	int32 v0 = screenMesh->addVertex(Vertex(fvec3(0.0F, 0.0F, 0.0F)));
	int32 v1 = screenMesh->addVertex(Vertex(fvec3(1.0F, 0.0F, 0.0F)));
	int32 v2 = screenMesh->addVertex(Vertex(fvec3(1.0F, 1.0F, 0.0F)));
	int32 v3 = screenMesh->addVertex(Vertex(fvec3(0.0F, 1.0F, 0.0F)));
	screenMesh->addFace(v0, v1, v2, v3);

	this->screenQuad = new GLMesh(screenMesh, screenAttributes);

	int32 width, height;
	Application::getWindowSize(&width, &height);
	this->setHistogram(256.0);
	this->setResolution(uvec2(width, height));
}

void ScreenRenderer::updateHistogram(double dt) {

	this->histogramBuffer->bind(this->histogramBinCount, 1);
	glEnable(GL_TEXTURE_2D);

	glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->histogramShader->useProgram(true);
	this->histogramShader->setUniform("textureSampler", 0);
	this->histogramShader->setUniform("textureSize", fvec2(this->screenResolution));
	this->histogramShader->setUniform("histogramSize", fvec2(this->histogramResolution));
	this->histogramShader->setUniform("histogramBinCount", int32(this->histogramBinCount));
	this->histogramShader->setUniform("histogramBrightnessRange", int32(this->histogramBrightnessRange));

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (int i = 0; i < 4; i++) {
		this->histogramShader->setUniform("channel", i);
		this->histogramPoints->draw();
	}
	glDisable(GL_BLEND);


	// Histogram pixel readback request

	for (int i = 0; i < this->histogramTransferBufferCount; i++) {
		uint32 readSize = this->histogramBinCount * sizeof(fvec4);
		uint32 readOffset = i * readSize;

		if (this->histogramTransferSync[i] == NULL) {
			// Make a readback request in the first available buffer.
			glBindBuffer(GL_PIXEL_PACK_BUFFER, this->histogramTransferBuffer);
			glBindTexture(GL_TEXTURE_2D, this->histogramTexture);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (void*)readOffset);

			this->histogramTransferSync[i] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		} else {
			// Check if any readbacks have been signalled.
			int32 result;
			glGetSynciv(this->histogramTransferSync[i], GL_SYNC_STATUS, sizeof(result), NULL, &result);

			if (result == GL_SIGNALED) {
				// Read the data back and copy it.
				glBindBuffer(GL_PIXEL_PACK_BUFFER, this->histogramTransferBuffer);
				vec4* data = static_cast<vec4*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, readOffset, readSize, GL_MAP_READ_BIT));
				std::memcpy(this->histogramData, data, readSize);

				glDeleteSync(this->histogramTransferSync[i]);
				this->histogramTransferSync[i] = NULL;

				glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

				// Build the normalized cumulative distribution function
				for (int i = 0; i < this->histogramBinCount; i++) {
					// Normalized CDF
					fvec4 data = this->histogramData[i] / (float)(this->histogramResolution.x * this->histogramResolution.y);

					if (i == 0) {
						this->histogramCumulativeDistribution[i] = data;
					} else {
						this->histogramCumulativeDistribution[i] = data + this->histogramCumulativeDistribution[i - 1];
					}
				}

				// Update CDF texture GPU-side
				glBindTexture(GL_TEXTURE_2D, this->histogramCumulativeTexture);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->histogramBinCount, 1, GL_RGBA, GL_FLOAT, (void*) this->histogramCumulativeDistribution);

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
				for (int i = 0; i < this->histogramBinCount; i++) {
					maxHistogramValue = glm::max(maxHistogramValue, this->histogramData[i].a);
				}
				float invMaxHistogramValue = 1.0F / maxHistogramValue;

				float sumLuminance = 0.0;
				for (int i = 0; i < this->histogramBinCount; i++) {
					sumLuminance += this->histogramData[i].a * invMaxHistogramValue;
				}

				fvec4 filter = fvec4(0.0, 0.0, sumLuminance * lowPercent, sumLuminance * highPercent);

				for (int i = 0; i < this->histogramBinCount; i++) {
					float t = (float)i / (float)this->histogramBinCount;
					float binVal = this->histogramData[i].a * invMaxHistogramValue;
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

	// Update the current screen exposure over time.
	float deltaExposure = this->expectedScreenExposure - this->currScreenExposure;
	float eyeAdaptationRate = deltaExposure < 0.0 ? this->eyeAdaptationRateDecr : this->eyeAdaptationRateIncr;
	this->currScreenExposure += (deltaExposure) * (1.0 - exp(-dt * eyeAdaptationRate));
}

void ScreenRenderer::render(double partialTicks, double dt) {
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


	// Render histogram for current frame.
	this->updateHistogram(dt);


	// Render final scene.
	FrameBuffer::unbind();

	this->screenShader->useProgram(true);
	SCENE_GRAPH.applyUniforms(this->screenShader);
	this->applyUniforms(this->screenShader);

	this->screenShader->setUniform("albedoTexture", 0);
	this->screenShader->setUniform("normalTexture", 1);
	this->screenShader->setUniform("positionTexture", 2);
	this->screenShader->setUniform("specularEmission", 3);
	this->screenShader->setUniform("depthTexture", 4);

	this->screenShader->setUniform("histogramTexture", 5);
	this->screenShader->setUniform("histogramCumulativeTexture", 6);

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->normalTexture);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->positionTexture);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->specularEmissionTexture);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->depthTexture);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, this->histogramTexture);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, this->histogramCumulativeTexture);

	this->screenQuad->draw();
}

void ScreenRenderer::applyUniforms(ShaderProgram* program) {
	program->setUniform("msaaSamples", int32(this->msaaSamples));
	program->setUniform("histogramBinCount", int32(this->histogramBinCount));
	program->setUniform("histogramDownsample", int32(this->histogramDownsample));
	program->setUniform("screenResolution", fvec2(this->screenResolution));
	program->setUniform("histogramResolution", fvec2(this->histogramResolution));
	program->setUniform("screenExposureMultiplier", this->currScreenExposure);
}

void ScreenRenderer::bindScreenBuffer() const {
	this->screenBuffer->bind(this->screenResolution.x, this->screenResolution.y);
}

bool ScreenRenderer::setHistogram(uint32 binCount) {

	if (binCount < 5) {
		logError("Cannot initialize histogram with too few bins");
		return false;
	}

	this->histogramBinCount = binCount;

	delete this->histogramBuffer;

	glEnable(GL_TEXTURE_2D);

	glDeleteTextures(1, &this->histogramTexture);
	glGenTextures(1, &this->histogramTexture);

	glDeleteTextures(1, &this->histogramCumulativeTexture);
	glGenTextures(1, &this->histogramCumulativeTexture);

	glBindTexture(GL_TEXTURE_2D, this->histogramTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->histogramBinCount, 1, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, this->histogramCumulativeTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->histogramBinCount, 1, 0, GL_RGBA, GL_FLOAT, NULL);

	//uint32* drawBuffers = new uint32[1]{ GL_COLOR_ATTACHMENT0 };

	this->histogramBuffer = new FrameBuffer();
	this->histogramBuffer->bind(this->histogramBinCount, 1);
	//this->histogramBuffer->setDrawBuffers(1, drawBuffers);
	//delete[] drawBuffers;

	this->histogramBuffer->createColourTextureAttachment(0, this->histogramTexture, GL_TEXTURE_2D);
	logInfo("Validating histogram framebuffer");
	this->histogramBuffer->checkStatus(true);


	glGenBuffers(1, &this->histogramTransferBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->histogramTransferBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, this->histogramTransferBufferCount * this->histogramBinCount * sizeof(fvec4), NULL, GL_STREAM_COPY);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	delete[] this->histogramData;
	delete[] this->histogramCumulativeDistribution;
	this->histogramData = new fvec4[this->histogramBinCount]();
	this->histogramCumulativeDistribution = new fvec4[this->histogramBinCount]();

	return true;
}

bool ScreenRenderer::setResolution(uvec2 resolution) {
	if (resolution != this->screenResolution) {
		if (resolution.x < 100 || resolution.y < 100) { // 100 x 100 is an arbitary minimum resolution. why would anyone even want anything less?
			logError("Cannot initialize screen renderer with resolution of [%d x %d]", resolution.x, resolution.y);
			return false;
		}

		this->screenResolution = resolution;

		delete this->screenBuffer;

		glEnable(GL_TEXTURE_2D);

		glDeleteTextures(1, &this->albedoTexture);
		glGenTextures(1, &this->albedoTexture);

		glDeleteTextures(1, &this->normalTexture);
		glGenTextures(1, &this->normalTexture);

		glDeleteTextures(1, &this->positionTexture);
		glGenTextures(1, &this->positionTexture);

		glDeleteTextures(1, &this->specularEmissionTexture);
		glGenTextures(1, &this->specularEmissionTexture);

		glDeleteTextures(1, &this->depthTexture);
		glGenTextures(1, &this->depthTexture);

		uint32* drawBuffers = new uint32[3]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

		// Screen
		this->screenBuffer = new FrameBuffer();
		this->screenBuffer->bind(this->screenResolution.x, this->screenResolution.y);
		this->screenBuffer->setDrawBuffers(3, drawBuffers);
		delete[] drawBuffers;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGBA32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->normalTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->positionTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->specularEmissionTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RG32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->depthTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_DEPTH_COMPONENT32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		this->screenBuffer->createColourTextureAttachment(0, this->albedoTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(1, this->normalTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(2, this->positionTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(3, this->specularEmissionTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createDepthTextureAttachment(this->depthTexture, GL_TEXTURE_2D_MULTISAMPLE);
		logInfo("Validating screen framebuffer");
		this->screenBuffer->checkStatus(true);











		VertexLayout histogramPointAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

		this->histogramResolution = uvec2(this->screenResolution / this->histogramDownsample);
		int32 vertexCount = this->histogramResolution.x * this->histogramResolution.y;

		uint64 a = Time::now();
		MeshData* histogramPointMesh = new MeshData(vertexCount, vertexCount, histogramPointAttributes);

		fvec3 vertex = fvec3(0.0, 0.0, 0.0);
		for (int i = 0; i < vertexCount; i++) {
			vertex.x = (float)(i / this->histogramResolution.y) / (float)this->histogramResolution.x;
			vertex.y = (float)(i % this->histogramResolution.y) / (float)this->histogramResolution.y;
			// vertices.push_back(Vertex(vertex));
			// indices.push_back(i);
			histogramPointMesh->addIndex(histogramPointMesh->addVertex(vertex));
		}

		uint64 b = Time::now();
		logInfo("Took %f ms to initialize histogram with %d vertices", (b - a) / 1000000.0, vertexCount);

		this->histogramPoints = new GLMesh(histogramPointMesh, histogramPointAttributes);
		this->histogramPoints->setPrimitive(GL_POINTS);

		FrameBuffer::unbind();

		return true;
	}
}

uvec2 ScreenRenderer::getResolution() const {
	return this->screenResolution;
}

uint32 ScreenRenderer::getAlbedoTexture() {
	return this->albedoTexture;
}

uint32 ScreenRenderer::getNormalTexture() {
	return this->normalTexture;
}

uint32 ScreenRenderer::getPositionTexture() {
	return this->positionTexture;
}

uint32 ScreenRenderer::getSpecularEmissionTexture() {
	return this->specularEmissionTexture;
}

uint32 ScreenRenderer::getDepthTexture() {
	return this->depthTexture;
}
