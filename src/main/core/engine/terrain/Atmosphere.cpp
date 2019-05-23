#include "Atmosphere.h"
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


Atmosphere::Atmosphere(Planet* planet, float atmosphereHeight, float rayleighHeight, float mieHeight, fvec3 rayleighWavelength, fvec3 mieWavelength):
	planet(planet),
	atmosphereHeight(atmosphereHeight),
	rayleighHeight(rayleighHeight),
	mieHeight(mieHeight),
	rayleighWavelength(rayleighWavelength),
	mieWavelength(mieWavelength) {

	this->sunDirection = fvec3(0.26726F, 0.8018F, 0.5345F);
	this->sunIntensity = 22.0F;
}

Atmosphere::~Atmosphere() {

}

Planet* Atmosphere::getPlanet() const {
	return this->planet;
}

float Atmosphere::getAtmosphereHeight() const {
	return this->atmosphereHeight;
}

void Atmosphere::setAtmosphereHeight(float atmosphereHeight) {
	this->atmosphereHeight = atmosphereHeight;
}

float Atmosphere::getRayleighHeight() const {
	return this->rayleighHeight;
}

void Atmosphere::setRayleighHeight(float rayleighHeight) {
	this->rayleighHeight = rayleighHeight;
}

float Atmosphere::getMieHeight() const {
	return this->mieHeight;
}

void Atmosphere::setMieHeight(float mieHeight) {
	this->mieHeight = mieHeight;
}

float Atmosphere::getSunIntensity() const {
	return this->sunIntensity;
}

void Atmosphere::setSunIntensity(float sunIntensity) {
	this->sunIntensity = sunIntensity;
}

fvec3 Atmosphere::getSunDirection() const {
	return this->sunDirection;
}

void Atmosphere::setSunDirection(fvec3 sunDirection) {
	this->sunDirection = sunDirection;
}

fvec3 Atmosphere::getRayleighWavelength() const {
	return this->rayleighWavelength;
}

void Atmosphere::setRayleighWavelength(fvec3 rayleighWavelength) {
	this->rayleighWavelength = rayleighWavelength;
}

fvec3 Atmosphere::getMieWavelength() const {
	return this->mieWavelength;
}

void Atmosphere::setMieWavelength(fvec3 mieWavelength) {
	this->mieWavelength = mieWavelength;
}
