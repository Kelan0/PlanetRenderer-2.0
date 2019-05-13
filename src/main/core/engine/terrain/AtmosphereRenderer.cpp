#include "AtmosphereRenderer.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/terrain/Planet.h"
#include "core/application/Application.h"
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
}

AtmosphereRenderer::~AtmosphereRenderer() {
	delete this->atmosphereProgram;
	delete this->screenQuad;
}

void AtmosphereRenderer::render(double partialTicks, double dt) {
	this->atmosphereProgram->useProgram(true);
	SCENE_GRAPH.applyUniforms(this->atmosphereProgram);
	this->planet->applyUniforms(this->atmosphereProgram);

	this->atmosphereProgram->setUniform("localCameraPosition", (fvec3)(this->planet->getLocalCameraPosition() * 1000.0));
	this->atmosphereProgram->setUniform("innerRadius", (float) (this->planet->getRadius()) * 1000.0F);
	this->atmosphereProgram->setUniform("outerRadius", (float) (this->planet->getRadius() + this->atmosphereHeight) * 1000.0F);
	this->atmosphereProgram->setUniform("rayleighHeight", (float) (this->rayleighHeight) * 1000.0F);
	this->atmosphereProgram->setUniform("mieHeight", (float) (this->mieHeight) * 1000.0F);
	this->atmosphereProgram->setUniform("sunIntensity", (float) (this->sunIntensity));
	this->atmosphereProgram->setUniform("sunDirection", (fvec3) (this->sunDirection));
	this->atmosphereProgram->setUniform("rayleighWavelength", (fvec3) (this->rayleighWavelength));
	this->atmosphereProgram->setUniform("mieWavelength", (fvec3) (this->mieWavelength));

	this->screenQuad->draw();
	this->atmosphereProgram->useProgram(false);
}

void AtmosphereRenderer::update(double dt) {

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
