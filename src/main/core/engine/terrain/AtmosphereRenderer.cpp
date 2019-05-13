#include "AtmosphereRenderer.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/terrain/Planet.h"
#include "core/application/Application.h"
#include <GL/glew.h>


AtmosphereRenderer::AtmosphereRenderer(Planet* planet) {
	this->planet = planet;

	this->atmosphereProgram = new ShaderProgram();
	this->atmosphereProgram->addShader(GL_VERTEX_SHADER, "atmosphere/vert.glsl");
	this->atmosphereProgram->addShader(GL_FRAGMENT_SHADER, "atmosphere/frag.glsl");
	this->atmosphereProgram->addAttribute(0, "vs_vertexPosition");
	this->atmosphereProgram->completeProgram();

	VertexLayout screenAttributes = VertexLayout(8, { VertexAttribute(0, 2, 0) }, 
		[](Vertex v) -> std::vector<float> {
			return std::vector<float> {float(v.position.x), float(v.position.y)}; 
		}
	);

	MeshData* screenMesh = new MeshData(4, 6, screenAttributes);
	int32 v0 = screenMesh->addVertex(Vertex(fvec3(0.0F, 0.0F, 0.0F)));
	int32 v1 = screenMesh->addVertex(Vertex(fvec3(1.0F, 0.0F, 0.0F)));
	int32 v2 = screenMesh->addVertex(Vertex(fvec3(1.0F, 1.0F, 0.0F)));
	int32 v3 = screenMesh->addVertex(Vertex(fvec3(0.0F, 1.0F, 0.0F)));
	screenMesh->addFace(v0, v1, v2, v3);

	this->screenQuad = new GLMesh(screenMesh, screenAttributes);

	this->wavelength = fvec3(0.850F, 0.770F, 0.665F);
	this->scaleDepth = 8.0F;
	this->eSun = 22.0F;
	this->kr = 0.0010F;
	this->km = 0.0025F;
	this->g = -0.9999F;
}


AtmosphereRenderer::~AtmosphereRenderer() {
	delete this->atmosphereProgram;
	delete this->screenQuad;
}

void AtmosphereRenderer::render(double partialTicks, double dt) {
	this->atmosphereProgram->useProgram(true);
	SCENE_GRAPH.applyUniforms(this->atmosphereProgram);
	this->planet->applyUniforms(this->atmosphereProgram);

	this->atmosphereProgram->setUniform("innerRadius", (float) (this->planet->getRadius()));
	this->atmosphereProgram->setUniform("outerRadius", (float) ((this->planet->getRadius() + 100.0)));
	this->atmosphereProgram->setUniform("invWavelength", fvec3(1.0F / glm::pow(this->wavelength, fvec3(4.0))));
	this->atmosphereProgram->setUniform("eSun", (float)this->eSun);
	this->atmosphereProgram->setUniform("kr", (float) this->kr);
	this->atmosphereProgram->setUniform("km", (float)this->km);
	this->atmosphereProgram->setUniform("scaleDepth", (float)this->scaleDepth);
	this->atmosphereProgram->setUniform("g", (float)this->g);

	this->screenQuad->draw();
	this->atmosphereProgram->useProgram(false);
}

void AtmosphereRenderer::update(double dt) {

}
