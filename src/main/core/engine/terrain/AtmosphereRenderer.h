#pragma once
#include "core/Core.h"

class FrameBuffer;
class ShaderProgram;
class GLMesh;
class Planet;

class AtmosphereRenderer
{
private:
	ShaderProgram* atmosphereProgram;
	GLMesh* screenQuad;
	Planet* planet;

	float atmosphereHeight; // The ehight of the atmosphere in kilometers above sea level.
	float rayleighHeight; // The scale height for rayleigh scattering. The scale height is generally the height at which the average atmospheric density can be found.
	float mieHeight; // The scale height for mie scattering. Mie and Rayleigh use different scale heights, since they occur at different altitudes.

	float sunIntensity; // The intensity of the sun.

	fvec3 sunDirection; // The direction from the planet to the light source.

	fvec3 rayleighWavelength; // The peak scattered wavelengths, for red green and blue, for the rayleigh scattering. Generally more blue gets scattered, for earthlike planets.
	fvec3 mieWavelength; // The peak scattered wavelengths, for red green and blue, for mie scattering. Generally r g and b scatter the same amount.
	ivec2 screenResolution;

	FrameBuffer* screenBuffer;
	uint32 atmosphereColourTexture;
	uint32 atmosphereBloomTexture;
public:
	AtmosphereRenderer(Planet* planet, float atmosphereHeight = 150.0, float rayleighHeight = 8.0, float mieHeight = 1.2, fvec3 rayleighWavelength = fvec3(3.8e-6f, 13.5e-6f, 33.1e-6f), fvec3 mieWavelength = fvec3(21e-6f));

	~AtmosphereRenderer();

	void render(double partialTicks, double dt);

	void update(double dt);

	void setResolution(uvec2 resolution);

	float getAtmosphereHeight() const;

	void setAtmosphereHeight(float atmosphereHeight);

	float getRayleighHeight() const;

	void setRayleighHeight(float rayleighHeight);

	float getMieHeight() const;

	void setMieHeight(float mieHeight);

	fvec3 getSunDirection() const;

	void setSunDirection(fvec3 sunDirection);

	fvec3 getRayleighWavelength() const;

	void setRayleighWavelength(fvec3 rayleighWavelength);

	fvec3 getMieWavelength() const;

	void setMieWavelength(fvec3 mieWavelength);
};

