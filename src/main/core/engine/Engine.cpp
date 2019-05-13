#include "Engine.h"
#include "core/application/Application.h"
#include <GL/glew.h>

double currentTickDelta = 0.0;
double currentFrameDelta = 0.0;
double smoothedTickDelta = 0.0;
double smoothedFrameDelta = 0.0;

double deltaSmoothingFactor = 0.001;

Engine::Engine() {

}


Engine::~Engine() {

}

void Engine::updateTickDeltaHistory(double dt) {
	currentTickDelta = dt;
	smoothedTickDelta = (1.0 - deltaSmoothingFactor) * smoothedTickDelta + deltaSmoothingFactor * currentTickDelta;
}

void Engine::updateFrameDeltaHistory(double dt) {
	currentFrameDelta = dt;
	smoothedFrameDelta = (1.0 - deltaSmoothingFactor) * smoothedFrameDelta + deltaSmoothingFactor * currentFrameDelta;
}

double Engine::getCurrentTickDelta(bool smoothed) {
	return smoothed ? smoothedTickDelta : currentTickDelta;
}

double Engine::getCurrentFrameDelta(bool smoothed) {
	return smoothed ? smoothedFrameDelta : currentFrameDelta;
}

void Engine::update(double dt) {
	//logInfo("%f updates per second\n", 1.0 / dt);
}

void Engine::render(double partialTicks, double dt) {
	//logInfo("%f frames per second, interp = %f\n", 1.0 / dt, partialTicks);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//glClearColor(0.1, 0.2, 0.4, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
}
