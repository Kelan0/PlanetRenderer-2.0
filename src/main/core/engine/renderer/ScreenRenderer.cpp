#include "ScreenRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"

#include <GL/glew.h>


ScreenRenderer::ScreenRenderer() {

	this->screenShader = new ShaderProgram();
	this->screenShader->addShader(GL_VERTEX_SHADER, "screen/vert.glsl");
	this->screenShader->addShader(GL_FRAGMENT_SHADER, "screen/frag.glsl");
	this->screenShader->addAttribute(0, "vs_vertexPosition");
	this->screenShader->completeProgram();

	VertexLayout screenAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

	MeshData* screenMesh = new MeshData(4, 6, screenAttributes);
	int32 v0 = screenMesh->addVertex(Vertex(fvec3(0.0F, 0.0F, 0.0F)));
	int32 v1 = screenMesh->addVertex(Vertex(fvec3(1.0F, 0.0F, 0.0F)));
	int32 v2 = screenMesh->addVertex(Vertex(fvec3(1.0F, 1.0F, 0.0F)));
	int32 v3 = screenMesh->addVertex(Vertex(fvec3(0.0F, 1.0F, 0.0F)));
	screenMesh->addFace(v0, v1, v2, v3);

	this->screenQuad = new GLMesh(screenMesh, screenAttributes);

	this->msaaSamples = 4;
	this->fixedSampleLocations = true;
}


ScreenRenderer::~ScreenRenderer() {
}

void ScreenRenderer::init(int windowWidth, int windowHeight) {
	delete this->screenBuffer;

	glDeleteTextures(1, &this->albedoTexture);
	glGenTextures(1, &this->albedoTexture);

	glDeleteTextures(1, &this->normalTexture);
	glGenTextures(1, &this->normalTexture);

	glDeleteTextures(1, &this->specularEmissionTexture);
	glGenTextures(1, &this->specularEmissionTexture);

	glDeleteTextures(1, &this->depthTexture);
	glGenTextures(1, &this->depthTexture);

	uint32* drawBuffers = new uint32[3]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

	// Screen
	this->screenBuffer = new FrameBuffer();
	this->screenBuffer->bind(windowWidth, windowHeight);
	this->screenBuffer->setDrawBuffers(3, drawBuffers);

	this->updateTexturesMultisample(windowWidth, windowHeight);

	this->screenBuffer->createColourTextureAttachment(0, this->albedoTexture, GL_TEXTURE_2D_MULTISAMPLE);
	this->screenBuffer->createColourTextureAttachment(1, this->normalTexture, GL_TEXTURE_2D_MULTISAMPLE);
	this->screenBuffer->createColourTextureAttachment(2, this->specularEmissionTexture, GL_TEXTURE_2D_MULTISAMPLE);
	this->screenBuffer->createDepthTextureAttachment(this->depthTexture, GL_TEXTURE_2D_MULTISAMPLE);

	FrameBuffer::unbind();

	delete[] drawBuffers;
}

void ScreenRenderer::updateTexturesMultisample(int32 width, int32 height) {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGBA32F, width, height, this->fixedSampleLocations);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->normalTexture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, width, height, this->fixedSampleLocations);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->specularEmissionTexture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RG32F, width, height, this->fixedSampleLocations);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->depthTexture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_DEPTH_COMPONENT32F, width, height, this->fixedSampleLocations);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}
