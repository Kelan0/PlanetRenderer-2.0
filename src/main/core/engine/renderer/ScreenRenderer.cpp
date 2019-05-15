#include "ScreenRenderer.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/FrameBuffer.h"
#include "core/util/Time.h"

#include <GL/glew.h>


ScreenRenderer::ScreenRenderer() {
	this->msaaSamples = 4;
	this->fixedSampleLocations = true;
	this->histogramBinCount = 256;
}


ScreenRenderer::~ScreenRenderer() {

}

void ScreenRenderer::init() {
	this->screenShader = new ShaderProgram();
	this->screenShader->addShader(GL_VERTEX_SHADER, "screen/vert.glsl");
	this->screenShader->addShader(GL_FRAGMENT_SHADER, "screen/frag.glsl");
	this->screenShader->addAttribute(0, "vs_vertexPosition");
	this->screenShader->completeProgram();

	this->histogramShader = new ShaderProgram();
	this->histogramShader->addShader(GL_VERTEX_SHADER, "histogram/vert.glsl");
	this->histogramShader->addShader(GL_FRAGMENT_SHADER, "histogram/frag.glsl");
	this->histogramShader->addAttribute(0, "vs_vertexPosition");
	this->histogramShader->completeProgram();

	VertexLayout screenAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

	MeshData* screenMesh = new MeshData(4, 6, screenAttributes);
	int32 v0 = screenMesh->addVertex(Vertex(fvec3(0.0F, 0.0F, 0.0F)));
	int32 v1 = screenMesh->addVertex(Vertex(fvec3(1.0F, 0.0F, 0.0F)));
	int32 v2 = screenMesh->addVertex(Vertex(fvec3(1.0F, 1.0F, 0.0F)));
	int32 v3 = screenMesh->addVertex(Vertex(fvec3(0.0F, 1.0F, 0.0F)));
	screenMesh->addFace(v0, v1, v2, v3);

	this->screenQuad = new GLMesh(screenMesh, screenAttributes);

	int32 width, height;
	Application::getWindowSize(&width, &height);
	this->setHistogram(256);
	this->setResolution(uvec2(width, height));
}

void ScreenRenderer::render(double partialTicks, double dt) {
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// HISTOGRAM

	this->histogramBuffer->bind(this->histogramBinCount, 1);
	glEnable(GL_TEXTURE_2D);

	glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->histogramShader->useProgram(true);
	this->histogramShader->setUniform("textureSampler", 0);
	this->histogramShader->setUniform("textureSize", fvec2(this->resolution));
	this->histogramShader->setUniform("histogramBins", int32(this->histogramBinCount));
	
	glEnable(GL_TEXTURE_2D_MULTISAMPLE);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);
	
	// TODO: MEASURE THIS PERFORMANCE AND MAKE NECESSARY OPTIMISATIONS.
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (int i = 0; i < 4; i++) {
		this->histogramShader->setUniform("channel", i);
		this->histogramPoints->draw();
	}
	glDisable(GL_BLEND);



	// POSTPROCESSING / FINAL RENDER
	
	FrameBuffer::unbind();

	this->screenShader->useProgram(true);
	this->screenShader->setUniform("msaaSamples", int32(this->msaaSamples));
	this->screenShader->setUniform("histogramBinCount", int32(this->histogramBinCount));
	this->screenShader->setUniform("resolution", fvec2(this->resolution));
	this->screenShader->setUniform("albedoTexture", 0);
	this->screenShader->setUniform("normalTexture", 1);
	this->screenShader->setUniform("specularEmission", 2);
	this->screenShader->setUniform("depthTexture", 3);

	this->screenShader->setUniform("histogramTexture", 4);

	glEnable(GL_TEXTURE_2D_MULTISAMPLE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->normalTexture);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->specularEmissionTexture);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->depthTexture);

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, this->histogramTexture);

	this->screenQuad->draw();
}

void ScreenRenderer::bindScreenBuffer() const {
	this->screenBuffer->bind(this->resolution.x, this->resolution.y);
}

bool ScreenRenderer::setHistogram(uint32 binCount) {

	if (binCount < 5) {
		logError("Cannot initialize histogram with too few bins");
		return false;
	}

	this->histogramBinCount = binCount;

	delete this->histogramBuffer;

	glEnable(GL_TEXTURE_2D);

	glDeleteTextures(1, &this->histogramTexture);
	glGenTextures(1, &this->histogramTexture);

	//uint32* drawBuffers = new uint32[1]{ GL_COLOR_ATTACHMENT0 };

	this->histogramBuffer = new FrameBuffer();
	this->histogramBuffer->bind(this->histogramBinCount, 1);
	//this->histogramBuffer->setDrawBuffers(1, drawBuffers);
	//delete[] drawBuffers;

	glBindTexture(GL_TEXTURE_2D, this->histogramTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->histogramBinCount, 1, 0, GL_RGBA, GL_FLOAT, NULL);

	this->histogramBuffer->createColourTextureAttachment(0, this->histogramTexture, GL_TEXTURE_2D);
	logInfo("Validating histogram framebuffer");
	this->histogramBuffer->checkStatus(true);
	return true;
}

bool ScreenRenderer::setResolution(uvec2 resolution) {
	if (resolution != this->resolution) {
		if (resolution.x < 100 || resolution.y < 100) { // 100 x 100 is an arbitary minimum resolution. why would anyone even want anything less?
			logError("Cannot initialize screen renderer with resolution of [%d x %d]", resolution.x, resolution.y);
			return false;
		}

		this->resolution = resolution;

		delete this->screenBuffer;

		glEnable(GL_TEXTURE_2D_MULTISAMPLE);

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
		this->screenBuffer->bind(this->resolution.x, this->resolution.y);
		this->screenBuffer->setDrawBuffers(3, drawBuffers);
		delete[] drawBuffers;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->albedoTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGBA32F, this->resolution.x, this->resolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->normalTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RGB32F, this->resolution.x, this->resolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->specularEmissionTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_RG32F, this->resolution.x, this->resolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->depthTexture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->msaaSamples, GL_DEPTH_COMPONENT32F, this->resolution.x, this->resolution.y, this->fixedSampleLocations);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		this->screenBuffer->createColourTextureAttachment(0, this->albedoTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(1, this->normalTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createColourTextureAttachment(2, this->specularEmissionTexture, GL_TEXTURE_2D_MULTISAMPLE);
		this->screenBuffer->createDepthTextureAttachment(this->depthTexture, GL_TEXTURE_2D_MULTISAMPLE);
		logInfo("Validating screen framebuffer");
		this->screenBuffer->checkStatus(true);











		VertexLayout histogramPointAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, [](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y)}; });

		this->histogramResolution = uvec2(this->resolution / 4u);
		int32 vertexCount = this->histogramResolution.x * this->histogramResolution.y;

		uint64 a = Time::now();
		MeshData* histogramPointMesh = new MeshData(vertexCount, vertexCount, histogramPointAttributes);

		fvec3 vertex = fvec3(0.0, 0.0, 0.0);
		for (int i = 0; i < vertexCount; i++) {
			vertex.x = (float)(i / this->histogramResolution.y) / (float)this->histogramResolution.x;
			vertex.y = (float)(i % this->histogramResolution.y) / (float)this->histogramResolution.y;
			// vertices.push_back(Vertex(vertex));
			// indices.push_back(i);
			histogramPointMesh->addIndex(histogramPointMesh->addVertex(vertex));
		}

		uint64 b = Time::now();
		logInfo("Took %f ms to initialize histogram with %d vertices", (b - a) / 1000000.0, vertexCount);

		this->histogramPoints = new GLMesh(histogramPointMesh, histogramPointAttributes);
		this->histogramPoints->setPrimitive(GL_POINTS);

		FrameBuffer::unbind();

		return true;
	}
}

uvec2 ScreenRenderer::getResolution() const {
	return this->resolution;
}
