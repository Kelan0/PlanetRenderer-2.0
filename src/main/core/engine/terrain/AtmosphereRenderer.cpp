#include "AtmosphereRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/FrameBuffer.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/ScreenRenderer.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/terrain/Planet.h"
#include "core/event/EventHandler.h"
#include <GL/glew.h>


AtmosphereRenderer::AtmosphereRenderer(Planet* planet, float atmosphereHeight, float rayleighHeight, float mieHeight, fvec3 rayleighWavelength, fvec3 mieWavelength):
	planet(planet),
	atmosphereHeight(atmosphereHeight),
	rayleighHeight(rayleighHeight),
	mieHeight(mieHeight),
	rayleighWavelength(rayleighWavelength),
	mieWavelength(mieWavelength) {

	this->atmosphereProgram = new ShaderProgram();
	this->atmosphereProgram->addShader(GL_VERTEX_SHADER, "atmosphere/vert.glsl");
	this->atmosphereProgram->addShader(GL_FRAGMENT_SHADER, "atmosphere/frag.glsl");
	this->atmosphereProgram->addAttribute(0, "vs_vertexPosition");
	this->atmosphereProgram->completeProgram();

	VertexLayout screenAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

	MeshData* screenMesh = new MeshData(4, 6, screenAttributes);
	int32 v0 = screenMesh->addVertex(Vertex(fvec3(0.0F, 0.0F, 0.0F)));
	int32 v1 = screenMesh->addVertex(Vertex(fvec3(1.0F, 0.0F, 0.0F)));
	int32 v2 = screenMesh->addVertex(Vertex(fvec3(1.0F, 1.0F, 0.0F)));
	int32 v3 = screenMesh->addVertex(Vertex(fvec3(0.0F, 1.0F, 0.0F)));
	screenMesh->addFace(v0, v1, v2, v3);

	this->screenQuad = new GLMesh(screenMesh, screenAttributes);

	this->sunDirection = fvec3(0.26726F, 0.8018F, 0.5345F);

	this->sunIntensity = 22.0F;

	// this->setResolution(uvec2(1600, 900)); // bad hard coded
	EVENT_HANDLER.subscribe(EventLambda(WindowResizeEvent) {
		this->setResolution(uvec2(event.newWidth, event.newHeight));
	});

}

AtmosphereRenderer::~AtmosphereRenderer() {
	delete this->atmosphereProgram;
	delete this->screenQuad;
}

void AtmosphereRenderer::render(double partialTicks, double dt) {

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	this->atmosphereProgram->useProgram(true);

	this->screenBuffer->bind(this->screenResolution.x, this->screenResolution.y);
	glClear(GL_COLOR_BUFFER_BIT);

	SCENE_GRAPH.applyUniforms(this->atmosphereProgram);
	SCREEN_RENDERER.applyUniforms(this->atmosphereProgram);
	this->planet->applyUniforms(this->atmosphereProgram);
	this->atmosphereProgram->setUniform("localCameraPosition", (fvec3)(this->planet->getLocalCameraPosition()) * 1000.0F);
	this->atmosphereProgram->setUniform("innerRadius", (float)(this->planet->getRadius()) * 1000.0F);
	this->atmosphereProgram->setUniform("outerRadius", (float)(this->planet->getRadius() + this->atmosphereHeight) * 1000.0F);
	this->atmosphereProgram->setUniform("rayleighHeight", (float)(this->rayleighHeight) * 1000.0F);
	this->atmosphereProgram->setUniform("mieHeight", (float)(this->mieHeight) * 1000.0F);
	this->atmosphereProgram->setUniform("sunIntensity", (float)(this->sunIntensity));
	this->atmosphereProgram->setUniform("sunDirection", (fvec3)(this->sunDirection));
	this->atmosphereProgram->setUniform("rayleighWavelength", (fvec3)(this->rayleighWavelength));
	this->atmosphereProgram->setUniform("mieWavelength", (fvec3)(this->mieWavelength));
	this->atmosphereProgram->setUniform("albedoTexture", 20);
	this->atmosphereProgram->setUniform("normalTexture", 21);
	this->atmosphereProgram->setUniform("positionTexture", 22);
	this->atmosphereProgram->setUniform("specularEmissionTexture", 23);
	this->atmosphereProgram->setUniform("depthTexture", 24);
	this->atmosphereProgram->setUniform("renderScreenQuad", false);

	glActiveTexture(GL_TEXTURE20);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, SCREEN_RENDERER.getAlbedoTexture());

	glActiveTexture(GL_TEXTURE21);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, SCREEN_RENDERER.getNormalTexture());

	glActiveTexture(GL_TEXTURE22);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, SCREEN_RENDERER.getPositionTexture());

	glActiveTexture(GL_TEXTURE23);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, SCREEN_RENDERER.getSpecularEmissionTexture());

	glActiveTexture(GL_TEXTURE24);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, SCREEN_RENDERER.getDepthTexture());

	this->screenQuad->draw();
	FrameBuffer::unbind();

	SCREEN_RENDERER.bindScreenBuffer();

	this->atmosphereProgram->setUniform("atmosphereScreenTexture", 0);
	this->atmosphereProgram->setUniform("renderScreenQuad", true);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->screenTexture);
	this->screenQuad->draw();
	glEnable(GL_DEPTH_TEST);
}

void AtmosphereRenderer::update(double dt) {

}

void AtmosphereRenderer::setResolution(uvec2 resolution) {
	this->screenResolution = resolution;

	delete this->screenBuffer;

	glEnable(GL_TEXTURE_2D);

	glDeleteTextures(1, &this->screenTexture);
	glGenTextures(1, &this->screenTexture);

	uint32* drawBuffers = new uint32[1]{ GL_COLOR_ATTACHMENT0 };

	this->screenBuffer = new FrameBuffer();
	this->screenBuffer->bind(this->screenResolution.x, this->screenResolution.y);
	this->screenBuffer->setDrawBuffers(1, drawBuffers);
	delete[] drawBuffers;

	glBindTexture(GL_TEXTURE_2D, this->screenTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->screenResolution.x, this->screenResolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->screenResolution.x, this->screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	this->screenBuffer->createColourTextureAttachment(0, this->screenTexture, GL_TEXTURE_2D);
	//this->screenBuffer->createDepthBufferAttachment(this->screenResolution.x, this->screenResolution.y, this->screenBuffer->genRenderBuffers());
	logInfo("Validating atmosphere framebuffer");

	this->screenBuffer->checkStatus(true);
}

float AtmosphereRenderer::getAtmosphereHeight() const {
	return this->atmosphereHeight;
}

void AtmosphereRenderer::setAtmosphereHeight(float atmosphereHeight) {
	this->atmosphereHeight = atmosphereHeight;
}

float AtmosphereRenderer::getRayleighHeight() const {
	return this->rayleighHeight;
}

void AtmosphereRenderer::setRayleighHeight(float rayleighHeight) {
	this->rayleighHeight = rayleighHeight;
}

float AtmosphereRenderer::getMieHeight() const {
	return this->mieHeight;
}

void AtmosphereRenderer::setMieHeight(float mieHeight) {
	this->mieHeight = mieHeight;
}

fvec3 AtmosphereRenderer::getSunDirection() const {
	return this->sunDirection;
}

void AtmosphereRenderer::setSunDirection(fvec3 sunDirection) {
	this->sunDirection = sunDirection;
}

fvec3 AtmosphereRenderer::getRayleighWavelength() const {
	return this->rayleighWavelength;
}

void AtmosphereRenderer::setRayleighWavelength(fvec3 rayleighWavelength) {
	this->rayleighWavelength = rayleighWavelength;
}

fvec3 AtmosphereRenderer::getMieWavelength() const {
	return this->mieWavelength;
}

void AtmosphereRenderer::setMieWavelength(fvec3 mieWavelength) {
	this->mieWavelength = mieWavelength;
}
