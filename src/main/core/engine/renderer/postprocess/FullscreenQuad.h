#pragma once
#include "core/Core.h"

class ShaderProgram;
class FrameBuffer;
class ShadferProgram;

class FullscreenQuad
{
private:
	ShaderProgram* screenProgram;
	FrameBuffer* screenBuffer;
	uint32* screenTextures;

	uvec2 resolution;
	std::map<std::string, uint32> renderTargets;

public:
	FullscreenQuad(ShaderProgram* program, uvec2 resolution, std::map<std::string, uint32> renderTargets);
	~FullscreenQuad();

	uint32 getTexture(std::string renderTarget);

	uvec2 getResolution();
};

