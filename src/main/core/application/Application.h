#pragma once

#include <typeindex>

#include "core/Core.h"
#include "core/util/Logger.h"

class Engine;
class EventHandler;
class ResourceHandler;
class InputHandler;
class SceneGraph;
class Logger;

// Maybe duplicating these definitions isn't a good idea...?
typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;

#define ENGINE Application::getEngine()
#define EVENT_HANDLER Application::getEventHandler()
#define RESOURCE_HANDLER Application::getResourceHandler()
#define INPUT_HANDLER Application::getInputHandler()
#define SCENE_GRAPH Application::getSceneGraph()
#define LOGGER Application::getLogger()

#define logInfo(str, ...) LOGGER.info(str, ##__VA_ARGS__)
#define logWarn(str, ...) LOGGER.warn(str, ##__VA_ARGS__)
#define logError(str, ...) LOGGER.error(str, ##__VA_ARGS__)
#define printf(str, ...) LOGGER.info(str, ##__VA_ARGS__)
#define glGetErrorString(error) LOGGER.getGLErrorString(error)

namespace Application
{
	/**
	 * Parse command line launch options, and setup low level engine and application settings,
	 * and initialize singletons.
	 */
	void setup(char* execpath, int argc, char* argv[]);

	/**
	 * Initialize the actual engine state, creating an OpenGL context, create threads, scene
	 * graph, load resources etc
	 */
	bool init();

	/**
	 * Start the actual engine running state. This function starts up the main loop and the
	 * appropriate threads, and does not return until the engine is finished running.
	 */
	void run();

	/**
	 * Cleanup the resources and memory left behind, shutdown threads, destroy scene graph etc
	 * after the engine has exited the main loop.
	 */
	void cleanup();

	/**
	 * Exit the application. If forceQuit is set to true, then the application will immediately
	 * exit the main loop and begin the cleanup process.
	 */
	void exit(bool forceQuit = false);

	void setRenderCallback(void(*callback)(double, double), bool pre = false);

	void setUpdateCallback(void(*callback)(double), bool pre = false);

	int32 getExitCode();

	void getWindowSize(int32* width, int32* height);

//	void getMousePosition(int32* x, int32* y);

	void setWindowSize(int32 width, int32 height);

	void setMousePosition(int32 x, int32 y);

	float getWindowAspectRatio();

	void setWindowTitle(std::string title);

	void setWindowFullscreen(bool fullscreen);

	void toggleWindowFullscreen();

	bool isWindowFullscreen();

	SDL_Window* getWindowHandle();

	SDL_GLContext getGLContext();

	Engine& getEngine();

	EventHandler& getEventHandler();

	ResourceHandler& getResourceHandler();

	InputHandler& getInputHandler();

	SceneGraph& getSceneGraph();

	Logger& getLogger();
};

