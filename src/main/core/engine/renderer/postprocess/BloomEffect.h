#pragma once
#include "core/Core.h"

class ShaderProgram;
class FrameBuffer;
class ShadferProgram;


class BloomEffect
{
private:
	FrameBuffer* screenBuffer;
	FrameBuffer* copyBuffer;

	ShaderProgram* blurShader;
	ShaderProgram* copyShader;
public:
	BloomEffect();
	~BloomEffect();
};

