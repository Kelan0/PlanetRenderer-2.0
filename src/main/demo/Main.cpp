
#include <iostream>
#include "core/application/Application.h"
#include "core/engine/geometry/Displacement.h"
#include "core/engine/geometry/MeshData.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/DebugRenderer.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/Camera.h"
#include "core/engine/scene/Transformation.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/scene/bounding/BoundingVolume.h"
#include "core/engine/scene/components/RenderComponent.h"
#include "core/engine/scene/components/FunctionComponent.h"
#include "core/engine/terrain/TerrainQuad.h"
#include "core/engine/terrain/TerrainRenderer.h"
#include "core/engine/terrain/Planet.h"
#include "core/util/InputHandler.h"
#include "core/util/Time.h"
#include <GL/glew.h>

Planet* planet;
double viewerMoveSpeed = 4.0;
dmat3 cameraAxis = dmat3(1.0);
bool resettingSpeed = false;
bool resettingFOV = false;

void init() {
	ShaderProgram* program = new ShaderProgram();
	program->addShader(GL_VERTEX_SHADER, "default/vert.glsl");
	program->addShader(GL_FRAGMENT_SHADER, "default/frag.glsl");
	program->addAttribute(0, "position");
	program->addAttribute(1, "normal");
	program->addAttribute(2, "texture");
	program->addAttribute(3, "colour");
	program->completeProgram();

	GLMesh* mesh = new GLMesh(MeshHelper::createCuboid());

	//GameObject* testCube = new GameObject(SCENE_GRAPH.getRoot(), "testCube", *((new Transformation())->rotate(vec3(1, 0, 0), QUARTER_PI)->rotate(vec3(0, 0, 1), //QUARTER_PI)));
	//testCube->addComponent("renderer", new RenderComponent(MeshHelper::createCuboid(), program));
	//testCube->addComponent("updater", new FunctionComponent(
	//	new FunctionComponent::RenderCallback([](SceneGraph* sceneGraph, GameObject* parent, double partialTicks, double dt) {
	//
	//}), new FunctionComponent::UpdateCallback([](SceneGraph* sceneGraph, GameObject* parent, double dt) {
	//	//parent->getTransformation().rotate(vec3(0, 1, 0), (float)(two_pi<float>() * dt * 0.125F));
	//})));

	float r = 6000;
	planet = new Planet(fvec3(), r, 2.0F, 24);
	SCENE_GRAPH.getRoot()->addChild("planet", planet);
	SCENE_GRAPH.getCamera()->setPosition(normalize(fvec3(0.0F, 1.0, -0.6)) * (r + 20.0F));

	//GameObject* testPlane = new GameObject(SCENE_GRAPH.getRoot(), "testPlane", Transformation(vec3(0.0F, -3.0F, 0.0F)));
	//
	//int sz = 32;
	//float** map = new float*[sz];
	//for (int i = 0; i < sz; i++) {
	//	map[i] = new float[sz];
	//	float x = (float)i / sz;
	//
	//	for (int j = 0; j < sz; j++) {
	//		float y = (float)j / sz;
	//		map[i][j] = sin(x * 8.0F) * sin(y * 8.0F);
	//	}
	//}

	//Displacement* displacement = new HeightMapDisplacement(sz, sz, map);

	//testPlane->addComponent("renderer", new RenderComponent(MeshHelper::createPlane(vec3(-4.0F), vec3(+4.0F), ivec2(16), mat3(1.0), true, displacement), program));
}

int fps = 0;
uint64 lastTime = 0;


void render(double partialTicks, double dt) {
	Camera* camera = SCENE_GRAPH.getCamera();

	fps++;

	if (lastTime == 0) {
		lastTime = Time::now();
	} else {
		uint64 now = Time::now();
		double elapsed = Time::time_cast<Time::time_unit, Time::seconds, double>(now - lastTime);

		if (elapsed >= 1.0) {
			logInfo("%d fps, %.3f GL/km, %.4f km altitude, move speed = %f", fps, Planet::scaleFactor, planet->getAltitude(planet->worldToLocalPoint(camera->getPosition())), viewerMoveSpeed);
			fps = 0;
			lastTime = now;
		}
	}


	dvec3 localCameraPosition = planet->worldToLocalPoint(camera->getPosition());
	//dvec3 sphc = planet->localToSphericalPoint(localCameraPosition);
	//CubeFace closestFace = planet->getClosestFaceWorld(camera->getPosition());
	//logInfo("Closest cube face = %s (%d), polar coordinates = [%f, %f]", 
	//	(closestFace == X_POS ? "X_POS" :
	//	closestFace == X_NEG ? "X_NEG" : 
	//	closestFace == Y_POS ? "Y_POS" :
	//	closestFace == Y_NEG ? "Y_NEG" :
	//	closestFace == Z_POS ? "Z_POS" :
	//	closestFace == Z_NEG ? "Z_NEG" : "UNKNOWN_FACE"), closestFace,
	//	glm::degrees(sphc.x), glm::degrees(sphc.y));

	//logInfo("Camera spherical coordinates = [%f, %f, %f]", glm::degrees(sphc.x), glm::degrees(sphc.y), sphc.z);

	// smoothly interpolate between global camera axis and lcoally deformed camera axis while camera is between certain altitides

	dvec3 z = camera->getForwardAxis();
	dvec3 y = camera->getUpAxis();
	dvec3 x = camera->getLeftAxis();

	bool globalAxis = true;

	dmat3 lookAxis = dmat3(x, y, z);
	if (globalAxis) {
		cameraAxis = lookAxis;
	} else {
		cameraAxis = planet->getLocalDeformation(localCameraPosition, lookAxis);
	}


	bool xInverted = dot(cross(cameraAxis[1], z), x) < 0.0;

	dvec3 moveVector = dvec3();
	double droll = 0.0;

	if (INPUT_HANDLER.keyDown(KEY_W)) moveVector += cameraAxis[2];
	if (INPUT_HANDLER.keyDown(KEY_S)) moveVector -= cameraAxis[2];
	if (INPUT_HANDLER.keyDown(KEY_D)) moveVector += xInverted ? -cameraAxis[0] : +cameraAxis[0];
	if (INPUT_HANDLER.keyDown(KEY_A)) moveVector -= xInverted ? -cameraAxis[0] : +cameraAxis[0];
	if (INPUT_HANDLER.keyDown(KEY_Q)) droll++;
	if (INPUT_HANDLER.keyDown(KEY_E)) droll--;
	if (INPUT_HANDLER.keyDown(KEY_SPACE)) moveVector += cameraAxis[1];
	if (INPUT_HANDLER.keyDown(KEY_LSHIFT)) moveVector -= cameraAxis[1];
	if (INPUT_HANDLER.keyDown(KEY_LCTRL) || INPUT_HANDLER.keyDown(KEY_RCTRL)) {
		if (INPUT_HANDLER.keyPressed(KEY_R)) resettingFOV = true;
		if (INPUT_HANDLER.keyDown(KEY_EQUALS)) {
			camera->setFieldOfView(glm::max(camera->getFieldOfView() / 1.005F, glm::radians(10.0F)));
			resettingFOV = false;
		}
		if (INPUT_HANDLER.keyDown(KEY_MINUS)) {
			camera->setFieldOfView(glm::min(camera->getFieldOfView() * 1.005F, glm::radians(170.0F)));
			resettingFOV = false;
		}
	} else {
		if (INPUT_HANDLER.keyPressed(KEY_R)) resettingSpeed = true;
		if (INPUT_HANDLER.keyPressed(KEY_EQUALS)) {
			viewerMoveSpeed = glm::min(viewerMoveSpeed * 1.5, 40000.0);
			resettingSpeed = false;
		}
		if (INPUT_HANDLER.keyPressed(KEY_MINUS)) {
			viewerMoveSpeed = glm::max(viewerMoveSpeed / 1.5, 0.05);
			resettingSpeed = false;
		}
	}

	if (resettingSpeed) {
		constexpr double desiredSpeed = 4.0;
		constexpr double transitionSpeed = 6.0;

		double d = (desiredSpeed - viewerMoveSpeed) * (dt * transitionSpeed);
		viewerMoveSpeed += d;

		if (fabs(viewerMoveSpeed - desiredSpeed) < 0.001) {
			viewerMoveSpeed = desiredSpeed;
			resettingSpeed = false;
		}
	}

	if (resettingFOV) {
		constexpr double desiredFOV = HALF_PI;
		constexpr double transitionSpeed = 6.0;

		double d = (desiredFOV - camera->getFieldOfView()) * (dt * transitionSpeed);
		camera->setFieldOfView(camera->getFieldOfView() + d);

		if (fabs(camera->getFieldOfView() - desiredFOV) < 0.001) {
			camera->setFieldOfView(desiredFOV);
			resettingFOV = false;
		}
	}

	if (INPUT_HANDLER.keyPressed(KEY_C)) {
		camera->setOrientation(planet->getLocalDeformation(localCameraPosition, camera->getAxis()));
	}

	if (length2(moveVector) > 1e-9) {
		camera->move(normalize(moveVector) * dt * viewerMoveSpeed / Planet::scaleFactor);
	}

	if (INPUT_HANDLER.keyPressed(KEY_F1)) SCENE_GRAPH.toggleWireframeMode();

	//dmat4 orientation = dmat4(dvec4(x, 0.0), dvec4(y, 0.0), dvec4(z, 0.0), dvec4(1.0));
	dquat orientation = camera->getOrientation();

	if (INPUT_HANDLER.isMouseGrabbed()) {
		dvec2 vel = INPUT_HANDLER.getMouseVelocity();

		dvec3 up = cameraAxis[1];
		dvec3 left = x;

		//double d = dot(left, cameraAxis[0]);
		//double ang = dt * (acos(d) - PI);

		double pitch = vel.y * 0.002;
		double yaw = vel.x * 0.002;
		double roll = droll * 0.9 * dt;

		orientation = angleAxis(pitch, left) * orientation;
		orientation = angleAxis(xInverted ? -yaw : +yaw, up) * orientation;
		orientation = angleAxis(roll, z) * orientation;
		//orientation = angleAxis(ang, z) * orientation;
	}

	camera->setOrientation(orientation);

	//planet->render(&SCENE_GRAPH, partialTicks, dt);

	std::vector<Vertex> vt = {
		Vertex(dvec3(0.0, 10000.0, 0.0)),
		Vertex(dvec3(0.0, -10000.0, 0.0)),
	};

	std::vector<int32> ix = {0, 1};

	DEBUG_RENDERER.begin(GL_LINES);
	DEBUG_RENDERER.setLightingEnabled(false);
	DEBUG_RENDERER.draw(vt, ix);
	DEBUG_RENDERER.finish();
}

void update(double dt) {
	//planet->update(&SCENE_GRAPH, dt);
}

int main(int argc, char* argv[]) {

	Time::TIMEZONE_OFFSET_MINUTES = 60; // British Summer Time (GMT+1)
	
	Application::setup(argv[0], argc - 1, &argv[1]);
	
	if (Application::init()) {
		init();
		Application::setWindowSize(1600, 900);
		Application::setRenderCallback(render);
		Application::setUpdateCallback(update);
		Application::run();
	}
	
	Application::cleanup();
	
	return Application::getExitCode();
}