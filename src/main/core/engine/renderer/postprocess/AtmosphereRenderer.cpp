#include "AtmosphereRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"
#include "core/engine/renderer/ScreenRenderer.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/terrain/Planet.h"
#include "core/engine/terrain/Atmosphere.h"
#include "core/util/Time.h"

#include <GL/glew.h>




AtmosphereRenderer::AtmosphereRenderer(ScreenRenderer* screenRenderer):
	screenRenderer(screenRenderer) {

	this->atmosphereProgram = new ShaderProgram();
	this->atmosphereProgram->addShader(GL_VERTEX_SHADER, "atmosphere/vert.glsl");
	this->atmosphereProgram->addShader(GL_FRAGMENT_SHADER, "atmosphere/frag.glsl");
	this->atmosphereProgram->addAttribute(0, "vs_vertexPosition");
	this->atmosphereProgram->completeProgram();
}

AtmosphereRenderer::~AtmosphereRenderer() {
	glDeleteTextures(1, &this->screenTexture);

	delete this->atmosphereProgram;
	delete this->atmosphereFrameBuffer;
}

void AtmosphereRenderer::initializeScreenResolution(uvec2 resolution) {
	this->screenResolution = resolution;

	delete this->atmosphereFrameBuffer;

	glEnable(GL_TEXTURE_2D);

	glDeleteTextures(1, &this->screenTexture);
	glGenTextures(1, &this->screenTexture);

	uint32* drawBuffers = new uint32[1]{ GL_COLOR_ATTACHMENT0 };

	this->atmosphereFrameBuffer = new FrameBuffer();
	this->atmosphereFrameBuffer->bind(this->screenResolution.x, this->screenResolution.y);
	this->atmosphereFrameBuffer->setDrawBuffers(1, drawBuffers);
	delete[] drawBuffers;

	glBindTexture(GL_TEXTURE_2D, this->screenTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->screenResolution.x, this->screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	this->atmosphereFrameBuffer->createColourTextureAttachment(0, this->screenTexture, GL_TEXTURE_2D);
	//this->atmosphereFrameBuffer->createDepthBufferAttachment(this->screenResolution.x, this->screenResolution.y, this->atmosphereFrameBuffer->genRenderBuffers());
	logInfo("Validating atmosphere framebuffer");

	this->atmosphereFrameBuffer->checkStatus(true);
}

bool AtmosphereRenderer::render(double partialTicks, double dt) {
	if (this->atmospheres.empty()) {
		return false;
	} else {
		this->atmosphereProgram->useProgram(true);

		this->atmosphereFrameBuffer->bind(this->screenResolution.x, this->screenResolution.y);
		glClear(GL_COLOR_BUFFER_BIT);

		SCENE_GRAPH.applyUniforms(this->atmosphereProgram);
		SCREEN_RENDERER.applyUniforms(this->atmosphereProgram);

		for (int i = 0; i < this->atmospheres.size(); i++) {
			Atmosphere* atmosphere = this->atmospheres[i];
			atmosphere->getPlanet()->applyUniforms(this->atmosphereProgram);
			this->atmosphereProgram->setUniform("localCameraPosition", (fvec3)(atmosphere->getPlanet()->getLocalCameraPosition()) * 1000.0F);
			this->atmosphereProgram->setUniform("innerRadius", (float)(atmosphere->getPlanet()->getRadius()) * 1000.0F);
			this->atmosphereProgram->setUniform("outerRadius", (float)(atmosphere->getPlanet()->getRadius() + atmosphere->getAtmosphereHeight()) * 1000.0F);
			this->atmosphereProgram->setUniform("rayleighHeight", (float)(atmosphere->getRayleighHeight()) * 1000.0F);
			this->atmosphereProgram->setUniform("mieHeight", (float)(atmosphere->getMieHeight()) * 1000.0F);
			this->atmosphereProgram->setUniform("sunIntensity", (float)(atmosphere->getSunIntensity()));
			this->atmosphereProgram->setUniform("sunDirection", (fvec3)(atmosphere->getSunDirection()));
			this->atmosphereProgram->setUniform("rayleighWavelength", (fvec3)(atmosphere->getRayleighWavelength()));
			this->atmosphereProgram->setUniform("mieWavelength", (fvec3)(atmosphere->getMieWavelength()));
			this->atmosphereProgram->setUniform("screenTexture", 20);
			this->atmosphereProgram->setUniform("positionTexture", 21);

			glActiveTexture(GL_TEXTURE20);
			glBindTexture(GL_TEXTURE_2D, this->screenRenderer->getScreenTexture());

			glActiveTexture(GL_TEXTURE21);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->screenRenderer->getPositionTexture());

			this->screenRenderer->getScreenQuad()->draw();
		}

		this->atmospheres.clear();
		return true;
	}
}

void AtmosphereRenderer::addAtmosphere(Atmosphere* atmosphere) {
	// TODO: depth sorting.
	if (atmosphere != NULL) {
		this->atmospheres.push_back(atmosphere);
	}
}

uvec2 AtmosphereRenderer::getScreenResolution() const {
	return this->screenResolution;
}

uint32 AtmosphereRenderer::getScreenTexture() const {
	return this->screenTexture;
}
