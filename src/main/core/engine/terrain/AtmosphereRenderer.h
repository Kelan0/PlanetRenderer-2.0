#pragma once
#include "core/Core.h"

class ShaderProgram;
class GLMesh;
class Planet;

class AtmosphereRenderer
{
private:
	ShaderProgram* atmosphereProgram;
	GLMesh* screenQuad;
	Planet* planet;

	fvec3 wavelength;
	float scaleDepth;
	float eSun;
	float kr;
	float km;
	float g;
public:
	AtmosphereRenderer(Planet* planet);
	~AtmosphereRenderer();

	void render(double partialTicks, double dt);

	void update(double dt);
};

