#include "FullscreenQuad.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"

#include <GL/glew.h>


FullscreenQuad::FullscreenQuad(ShaderProgram* program, uvec2 resolution, std::map<std::string, uint32> renderTargets):
	screenProgram(program), resolution(resolution), renderTargets(renderTargets) {

	glGenTextures(renderTargets.size(), this->screenTextures);
}

FullscreenQuad::~FullscreenQuad()
{
}

uint32 FullscreenQuad::getTexture(std::string renderTarget)
{
	return uint32();
}

uvec2 FullscreenQuad::getResolution()
{
	return uvec2();
}
