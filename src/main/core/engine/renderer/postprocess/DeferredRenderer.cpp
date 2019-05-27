#include "DeferredRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"
#include "core/engine/renderer/ScreenRenderer.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/util/Logger.h"

#include <GL/glew.h>

DeferredRenderer::DeferredRenderer(ScreenRenderer* screenRenderer):
	screenRenderer(screenRenderer) {
	this->screenTexture = 0;
	this->deferredFrameBuffer = NULL;

	this->deferredShader = new ShaderProgram();
	this->deferredShader->addShader(GL_VERTEX_SHADER, "deferred/vert.glsl");
	this->deferredShader->addShader(GL_FRAGMENT_SHADER, "deferred/frag.glsl");
	this->deferredShader->addAttribute(0, "vs_vertexPosition");
	this->deferredShader->completeProgram();
}

DeferredRenderer::~DeferredRenderer() {
	glDeleteTextures(1, &this->screenTexture);

	delete this->deferredShader;
	delete this->deferredFrameBuffer;
}

void DeferredRenderer::initializeScreenResolution(uvec2 screenResolution) {
	if (this->screenResolution != screenResolution) {
		this->screenResolution = screenResolution;

		glEnable(GL_TEXTURE_2D);

		glDeleteTextures(1, &this->screenTexture);
		glGenTextures(1, &this->screenTexture);

		uint32* drawBuffers = new uint32[1]{ GL_COLOR_ATTACHMENT0 };

		delete this->deferredFrameBuffer;
		this->deferredFrameBuffer = new FrameBuffer();
		this->deferredFrameBuffer->bind(this->screenResolution.x, this->screenResolution.y);
		this->deferredFrameBuffer->setDrawBuffers(1, drawBuffers);
		delete[] drawBuffers;

		glBindTexture(GL_TEXTURE_2D, this->screenTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->screenResolution.x, this->screenResolution.y, 0, GL_RGB, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		this->deferredFrameBuffer->createColourTextureAttachment(0, this->screenTexture, GL_TEXTURE_2D);
		logInfo("Validating deferred framebuffer");
		this->deferredFrameBuffer->checkStatus(true);
	}
}

bool DeferredRenderer::render(double partialTicks, double dt) {

	this->deferredFrameBuffer->bind(this->screenResolution.x, this->screenResolution.y);
	this->deferredShader->useProgram(true);
	SCENE_GRAPH.applyUniforms(this->deferredShader);

	this->deferredShader->setUniform("screenResolution", fvec2(this->screenResolution));
	this->deferredShader->setUniform("msaaSamples", int32(this->screenRenderer->getMSAASamples()));
	
	this->deferredShader->setUniform("albedoTexture", 0);
	this->deferredShader->setUniform("glowTexture", 1);
	this->deferredShader->setUniform("normalTexture", 2);
	this->deferredShader->setUniform("positionTexture", 3);
	this->deferredShader->setUniform("specularEmissionTexture", 4);
	this->deferredShader->setUniform("depthTexture", 5);
	
	glEnable(GL_TEXTURE_2D);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->screenRenderer->getAlbedoTexture());
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->screenRenderer->getGlowTexture());
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->screenRenderer->getNormalTexture());
	
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->screenRenderer->getPositionTexture());
	
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->screenRenderer->getSpecularEmissionTexture());
	
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->screenRenderer->getDepthTexture());
	
	this->screenRenderer->getScreenQuad()->draw();

	return true;
}

uvec2 DeferredRenderer::getScreenResolution() const {
	return this->screenResolution;
}

uint32 DeferredRenderer::getScreenTexture() const {
	return this->screenTexture;
}