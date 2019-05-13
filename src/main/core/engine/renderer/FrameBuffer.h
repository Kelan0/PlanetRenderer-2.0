#pragma once
#include "core/Core.h"

class FrameBuffer
{
private:
	uint32 frameBuffer;

public:
	FrameBuffer();
	~FrameBuffer();

	void bind(int32 width = -1, int32 height = -1);

	static void unbind();

	void setDrawBuffers(uint32 n, uint32* buffers);

	void createColourTextureAttachment(uint32 attachment, uint32 texture, uint32 target);

	void createDepthTextureAttachment(uint32 texture, uint32 target);

	void createColourBufferAttachment(int32 width, int32 height, uint32 colourbuffer, uint32 attachment, uint32 format);

	void createDepthBufferAttachment(int32 width, int32 height, uint32 depthBuffer);

	void createColourBufferAttachmentMultisample(int32 width, int32 height, uint32 colourbuffer, uint32 attachment, uint32 format, uint32 samples);

	void createDepthBufferAttachmentMultisample(int32 width, int32 height, uint32 depthBuffer, uint32 samples);

	uint32 genRenderBuffers();
};

