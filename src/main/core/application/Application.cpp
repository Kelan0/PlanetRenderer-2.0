
#include "core/application/Application.h"
#include "core/engine/renderer/DebugRenderer.h"
#include "core/engine/renderer/ScreenRenderer.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/event/EventHandler.h"
#include "core/util/ResourceHandler.h"
#include "core/util/InputHandler.h"
#include "core/util/Logger.h"
#include "core/util/Time.h"
#include "core/util/Timer.h"
#include <GL/glew.h>
#include <SDL.h>

namespace Application {
	bool running = true;
	bool started = false;

	EventHandler* eventHandler = NULL;
	ResourceHandler* resourceHandler = NULL;
	InputHandler* inputHandler = NULL;
	SceneGraph* sceneGraph = NULL;
	DebugRenderer* debugRenderer = NULL;
	ScreenRenderer* screenRenderer = NULL;
	Logger* logger = NULL;

	SDL_Window* window = NULL;
	SDL_GLContext context = NULL;
	// default resolution
	int windowWidth = 800;
	int windowHeight = 600;

	void(*preRenderCallback)(double, double);
	void(*postRenderCallback)(double, double);
	void(*preUpdateCallback)(double);
	void(*postUpdateCallback)(double);

	void Application::setup(char* execpath, int argc, char* argv[]) {
		eventHandler = new EventHandler();
		resourceHandler = new ResourceHandler(execpath);
		inputHandler = new InputHandler();
		sceneGraph = new SceneGraph();
		debugRenderer = new DebugRenderer();
		screenRenderer = new ScreenRenderer();
		logger = new Logger();
	}

	bool Application::init() {

		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			logError("Failed to initialize SDL\n%s\n", SDL_GetError());
			return false;
		}

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		window = SDL_CreateWindow("Unnamed Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_OPENGL);

		if (window == NULL) {
			logError("Failed to create SDL window\n%s\n", SDL_GetError());
			return false;
		}

		SDL_SetWindowResizable(window, SDL_TRUE);
		context = SDL_GL_CreateContext(window);

		GLenum error = glewInit();
		if (error != GLEW_OK) {
			logError("Failed to initialize OpenGL: %s", glewGetErrorString(error));
			return false;
		}

		logInfo("Initialized OpenGL context for version %s", glGetString(GL_VERSION));


		if (context == NULL) {
			logError("Failed to create SDL OpenGL context\n%s\n", SDL_GetError());
			return false;
		}

		SDL_GL_SetSwapInterval(0);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		
		EVENT_HANDLER.subscribe(EventLambda(WindowClosingEvent) {
			logInfo("Window closing");
			cleanup();
			exit();
		});

		EVENT_HANDLER.subscribe(EventLambda(WindowResizeEvent) {
			logInfo("Window resized");

			SCREEN_RENDERER.setResolution(uvec2(windowWidth, windowHeight));
		});

		INPUT_HANDLER.init();
		SCENE_GRAPH.init();
		DEBUG_RENDERER.init();
		SCREEN_RENDERER.init();

		return true;
	}

	void Application::run() {
		using namespace Time;

		uint64 lastTime = now();
		uint64 lastTick = lastTime;
		uint64 lastFrame = lastTime;

		double partialTicks = 0.0;
		double tdt = 1.0 / 60.0; // 0.016666...

		while (running) {
			uint64 time = now();
			uint64 elapsed = time - lastTime;
			double ldt = time_cast<time_unit, seconds, double>(elapsed);
			lastTime = time;

			partialTicks += ldt / tdt;

			// process events

			if (!started) {
				started = true;
				EVENT_HANDLER.enqueue(EngineStartedEvent());
			}

			INPUT_HANDLER.reset(ldt);

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					EVENT_HANDLER.fire(WindowClosingEvent());

					Timer::setTimeout([&](TimerEvent event, Subscription* subscription) {
						if (running) {
							running = false;
							logWarn("Shutdown was requested but the shutdown process exceeded the time limit. Forcing shutdown.");
						}
					}, 2.0);
				} else if (event.type == SDL_WINDOWEVENT) {
					if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						int32 oldWindowWidth = windowWidth;
						int32 oldWindowHeight = windowHeight;
						windowWidth = event.window.data1;
						windowHeight = event.window.data2;
						EVENT_HANDLER.fire(WindowResizeEvent(false, oldWindowWidth, oldWindowHeight, windowWidth, windowHeight));
					}
				} else if (event.type == SDL_KEYDOWN) {
					EVENT_HANDLER.fire(KeyboardEvent(PRESSED, 0, event.key.keysym.scancode));
				} else if (event.type == SDL_KEYUP) {
					EVENT_HANDLER.fire(KeyboardEvent(RELEASED, 0, event.key.keysym.scancode));
				} else if (event.type == SDL_MOUSEBUTTONDOWN) {
					EVENT_HANDLER.fire(MouseEvent(PRESSED, event.button.button, event.button.clicks, ivec2(event.button.x, event.button.y), ivec2(0, 0)));
				} else if (event.type == SDL_MOUSEBUTTONUP) {
					EVENT_HANDLER.fire(MouseEvent(RELEASED, event.button.button, event.button.clicks, ivec2(event.button.x, event.button.y), ivec2(event.motion.xrel, event.motion.yrel)));
				} else if (event.type == SDL_MOUSEMOTION) {
					EVENT_HANDLER.fire(MouseEvent(UNCHANGED, -1, 0, ivec2(event.motion.x, event.motion.y), ivec2(event.motion.xrel, event.motion.yrel)));
				}
			}

			EVENT_HANDLER.processQueue();

			// partialTicks will accumulate when ticks are missed... if it reaches 2 for example, two
			// ticks will run and the missed ticks will be caught up.

			// TODO: limit this, maybe 5 ticks to run at once, or dynamically decide based on
			// tick time history how many ticks are appropriate to run at once. If particalTicks
			// accumulates too much, log a warning for skipped ticks, and reset it to zero.
			while (partialTicks >= 1.0) {
				//engine->updateTickDeltaHistory(time_cast<time_unit, seconds, double>(time - lastTick));
				lastTick = time;

				if (preUpdateCallback != NULL) preUpdateCallback(tdt);
				sceneGraph->update(tdt);
				if (postUpdateCallback != NULL) postUpdateCallback(tdt);
				partialTicks--;
			}

			double fdt = time_cast<time_unit, seconds, double>(time - lastFrame);
			lastFrame = time;
			//engine->updateFrameDeltaHistory(fdt);

			screenRenderer->bindScreenBuffer();
			if (preRenderCallback != NULL) preRenderCallback(partialTicks, fdt);
			sceneGraph->render(partialTicks, fdt);
			if (postRenderCallback != NULL) postRenderCallback(partialTicks, fdt);
			screenRenderer->render(partialTicks, fdt);

			SDL_GL_SwapWindow(window);

			sleep(1);
		}
	}

	void Application::cleanup() {
		//delete eventHandler;
		//delete resourceHandler;
		//delete inputHandler;
		//delete sceneGraph;
		//delete debugRenderer;
		//delete screenRenderer;
		//delete logger;

	}

	void Application::exit(bool forceQuit) {
		//if (forceQuit || !eventHandler) {
		running = false;
		//} else {
		//	eventHandler->fire(APPLICATION_EXIT_EVENT);
		//}
	}

	void Application::setRenderCallback(void(*callback)(double, double), bool pre) {
		if (pre) {
			preRenderCallback = callback;
		} else {
			postRenderCallback = callback;
		}
	}

	void Application::setUpdateCallback(void(*callback)(double), bool pre) {
		if (pre) {
			preUpdateCallback = callback;
		} else {
			postUpdateCallback = callback;
		}
	}

	int Application::getExitCode() {
		return 0;
	}

	void Application::getWindowSize(int32* width, int32* height) {
		if (width != NULL) {
			*width = windowWidth;
		}

		if (height != NULL) {
			*height = windowHeight;
		}
	}

	void Application::setWindowSize(int32 width, int32 height) {
		SDL_SetWindowSize(window, width, height);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	void setMousePosition(int32 x, int32 y) {
		SDL_WarpMouseInWindow(window, x, y);
	}

	float Application::getWindowAspectRatio() {
		return (float) windowWidth / (float) windowHeight;
	}

	void setWindowTitle(std::string title) {
		SDL_SetWindowTitle(window, title.c_str());
	}

	void setWindowFullscreen(bool fullscreen) {
		SDL_SetWindowFullscreen(window, fullscreen);
	}

	void toggleWindowFullscreen() {
		setWindowFullscreen(!isWindowFullscreen());
	}

	bool isWindowFullscreen() {
		return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == SDL_WINDOW_FULLSCREEN;
	}

	SDL_Window* getWindowHandle() {
		return window;
	}

	SDL_GLContext getGLContext() {
		return context;
	}

	EventHandler& Application::getEventHandler() {
		return *eventHandler;
	}

	ResourceHandler& getResourceHandler() {
		return *resourceHandler;
	}

	InputHandler& getInputHandler() {
		return *inputHandler;
	}

	SceneGraph& getSceneGraph() {
		return *sceneGraph;
	}

	DebugRenderer& getDebugRenderer() {
		return *debugRenderer;
	}

	ScreenRenderer& getScreenRenderer() {
		return *screenRenderer;
	}

	Logger& Application::getLogger() {
		return *logger;
	}
}