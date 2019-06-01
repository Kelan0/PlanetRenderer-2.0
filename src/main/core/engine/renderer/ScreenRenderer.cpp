#include "ScreenRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"
#include "core/engine/renderer/postprocess/DeferredRenderer.h"
#include "core/engine/renderer/postprocess/AtmosphereRenderer.h"
#include "core/engine/renderer/postprocess/HistogramRenderer.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/terrain/Planet.h"
#include "core/util/Time.h"

#include <GL/glew.h>



ScreenRenderer::ScreenRenderer() {
	this->msaaSamples = 4;
	this->fixedSampleLocations = true;
	this->eyeAdaptationRateIncr = 2.0;
	this->eyeAdaptationRateDecr = 2.0;

	this->exposureEnabled = true;
	this->gammaCorrectionEnabled = true;
}


ScreenRenderer::~ScreenRenderer() {

}

void ScreenRenderer::init() {
	this->screenShader = new ShaderProgram();
	this->screenShader->addShader(GL_VERTEX_SHADER, "screen/vert.glsl");
	this->screenShader->addShader(GL_FRAGMENT_SHADER, "screen/frag.glsl");
	this->screenShader->addAttribute(0, "vs_vertexPosition");
	this->screenShader->completeProgram();

	VertexLayout screenAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

	MeshData* screenMesh = new MeshData(4, 6, screenAttributes);
	int32 v0 = screenMesh->addVertex(Vertex(fvec3(0.0F, 0.0F, 0.0F)));
	int32 v1 = screenMesh->addVertex(Vertex(fvec3(1.0F, 0.0F, 0.0F)));
	int32 v2 = screenMesh->addVertex(Vertex(fvec3(1.0F, 1.0F, 0.0F)));
	int32 v3 = screenMesh->addVertex(Vertex(fvec3(0.0F, 1.0F, 0.0F)));
	screenMesh->addFace(v0, v1, v2, v3);

	this->screenQuad = new GLMesh(screenMesh, screenAttributes);

	this->deferredRenderer = new DeferredRenderer(this);
	this->atmosphereRenderer = new AtmosphereRenderer(this);
	this->histogramRenderer = new HistogramRenderer(this);

	int32 width, height;
	Application::getWindowSize(&width, &height);
	this->setResolution(uvec2(width, height));
}

void ScreenRenderer::renderBloom() {

}

void ScreenRenderer::render(double partialTicks, double dt) {
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (this->deferredRenderer->render(partialTicks, dt)) {
		this->screenTexture = this->deferredRenderer->getScreenTexture();
	}

	if (this->atmosphereRenderer->render(partialTicks, dt)) {
		this->screenTexture = this->atmosphereRenderer->getScreenTexture();
	}

	this->histogramRenderer->render(partialTicks, dt);

	// Update the current screen exposure over time.
	float deltaExposure = this->histogramRenderer->getExpectedScreenExposure() - this->currScreenExposure;
	float eyeAdaptationRate = deltaExposure < 0.0 ? this->eyeAdaptationRateDecr : this->eyeAdaptationRateIncr;
	this->currScreenExposure += (deltaExposure) * (1.0 - exp(-dt * eyeAdaptationRate));



	// Render final scene.
	FrameBuffer::unbind();

	this->screenShader->useProgram(true);
	SCENE_GRAPH.applyUniforms(this->screenShader);
	this->histogramRenderer->applyUniforms(this->screenShader);
	this->applyUniforms(this->screenShader);

	this->screenShader->setUniform("albedoTexture", 0);
	this->screenShader->setUniform("glowTexture", 1);
	this->screenShader->setUniform("normalTexture", 2);
	this->screenShader->setUniform("positionTexture", 3);
	this->screenShader->setUniform("specularEmissionTexture", 4);
	this->screenShader->setUniform("depthTexture", 5);

	this->screenShader->setUniform("screenTexture", 6);
	this->screenShader->setUniform("histogramTexture", 7);
	this->screenShader->setUniform("histogramCumulativeTexture", 8);

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->glowTexture);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->normalTexture);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->positionTexture);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->specularEmissionTexture);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->depthTexture);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, this->screenTexture);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, this->histogramRenderer->getGraphTexture());

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, this->histogramRenderer->getCumulativeTexture());

	this->screenQuad->draw();
}

void ScreenRenderer::applyUniforms(ShaderProgram* program) {
	program->setUniform("msaaSamples", int32(this->msaaSamples));
	program->setUniform("screenResolution", fvec2(this->screenResolution));
	program->setUniform("screenExposureMultiplier", this->currScreenExposure);
	program->setUniform("exposureEnabled", this->exposureEnabled);
	program->setUniform("gammaCorrectionEnabled", this->gammaCorrectionEnabled);
	program->setUniform("directionToLight", fvec3(this->sunDirection));
}

void ScreenRenderer::bindScreenBuffer() const {
	this->screenBuffer->bind(this->screenResolution.x, this->screenResolution.y);
}

bool ScreenRenderer::setResolution(uvec2 resolution) {
	if (resolution != this->screenResolution) {
		if (resolution.x < 100 || resolution.y < 100) { // 100 x 100 is an arbitary minimum resolution. why would anyone even want anything less?
			logError("Cannot initialize screen renderer with resolution of [%d x %d]", resolution.x, resolution.y);
			return false;
		}

		this->screenResolution = resolution;
		int maxColorAttachments;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
		logInfo("Maximum framebuffer attachments: %d", maxColorAttachments);

		glEnable(GL_TEXTURE_2D);

		glDeleteTextures(1, &this->albedoTexture);
		glGenTextures(1, &this->albedoTexture);

		glDeleteTextures(1, &this->glowTexture);
		glGenTextures(1, &this->glowTexture);

		glDeleteTextures(1, &this->normalTexture);
		glGenTextures(1, &this->normalTexture);

		glDeleteTextures(1, &this->positionTexture);
		glGenTextures(1, &this->positionTexture);

		glDeleteTextures(1, &this->specularEmissionTexture);
		glGenTextures(1, &this->specularEmissionTexture);

		glDeleteTextures(1, &this->depthTexture);
		glGenTextures(1, &this->depthTexture);

		uint32* drawBuffers = new uint32[4]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

		// Screen
		delete this->screenBuffer;
		this->screenBuffer = new FrameBuffer();
		this->screenBuffer->bind(this->screenResolution.x, this->screenResolution.y);
		this->screenBuffer->setDrawBuffers(4, drawBuffers);
		delete[] drawBuffers;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGBA32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->normalTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->positionTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->specularEmissionTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->glowTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->depthTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_DEPTH_COMPONENT32F, this->screenResolution.x, this->screenResolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		this->screenBuffer->createColourTextureAttachment(0, this->albedoTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(1, this->normalTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(2, this->positionTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(3, this->specularEmissionTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(4, this->glowTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createDepthTextureAttachment(this->depthTexture, GL_TEXTURE_2D_MULTISAMPLE);
		logInfo("Validating screen framebuffer");
		this->screenBuffer->checkStatus(true);

		this->deferredRenderer->initializeScreenResolution(this->screenResolution);
		this->atmosphereRenderer->initializeScreenResolution(this->screenResolution);
		this->histogramRenderer->initializeScreenResolution(this->screenResolution);

		FrameBuffer::unbind();

		return true;
	}
}

uvec2 ScreenRenderer::getResolution() const {
	return this->screenResolution;
}

uint32 ScreenRenderer::getMSAASamples() const {
	return this->msaaSamples;
}

uint32 ScreenRenderer::getAlbedoTexture() {
	return this->albedoTexture;
}

uint32 ScreenRenderer::getGlowTexture() {
	return this->glowTexture;
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

uint32 ScreenRenderer::getScreenTexture() {
	return this->screenTexture;
}

GLMesh* ScreenRenderer::getScreenQuad() {
	return this->screenQuad;
}

dvec3 ScreenRenderer::getSunDirection() const {
	return this->sunDirection;
}

void ScreenRenderer::setExposureEnabled(bool enabled) {
	this->exposureEnabled = enabled;
}

bool ScreenRenderer::isExposureEnabled() const {
	return this->exposureEnabled;
}

void ScreenRenderer::setGammaCorrectionEnabled(bool enabled) {
	this->gammaCorrectionEnabled = enabled;
}

bool ScreenRenderer::isGammaCorrectionEnabled() const {
	return this->gammaCorrectionEnabled;
}


DeferredRenderer* ScreenRenderer::getDeferredRenderer() const {
	return this->deferredRenderer;
}

AtmosphereRenderer* ScreenRenderer::getAtmosphereRenderer() const {
	return this->atmosphereRenderer;
}

HistogramRenderer* ScreenRenderer::getHistogramRenderer() const {
	return this->histogramRenderer;
}

