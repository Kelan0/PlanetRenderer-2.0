#include "InputHandler.h"
#include "core/event/EventHandler.h"
#include "core/application/Application.h"
#include <SDL.h>

bool pressedKeys[KEYBOARD_SIZE];
bool activeKeys[KEYBOARD_SIZE];
bool releasedKeys[KEYBOARD_SIZE];

int pressedButtons[MOUSE_SIZE];
bool activeButtons[MOUSE_SIZE];
bool releasedButtons[MOUSE_SIZE];
bool doubleClickedButtons[MOUSE_SIZE];

std::vector<std::pair<uint64, ivec2>> previousMousePosition; // mouse position history over a short previous timestep, for smoothing the velocity
fvec2 lastMousePosition;
fvec2 currMousePosition;
fvec2 nextMousePosition;
fvec2 mouseVelocity;

uint64 lastEvent;
bool mouseGrabbed;

Subscription* keyboardSubscription;
Subscription* mouseSubscription;

InputHandler::InputHandler() {}


InputHandler::~InputHandler() {
	if (keyboardSubscription != NULL) {
		keyboardSubscription->unsubscribe();
		mouseSubscription->unsubscribe();
	}
}


void InputHandler::init() {
	keyboardSubscription = EVENT_HANDLER.subscribe(EventLambda(KeyboardEvent) {
		if (event.state == PRESSED) {
			if (!activeKeys[event.keycode])
				pressedKeys[event.keycode] = true;
			activeKeys[event.keycode] = true;
		} else if (event.state == RELEASED) {
			pressedKeys[event.keycode] = false;
			activeKeys[event.keycode] = false;
			releasedKeys[event.keycode] = true;
		}
	});

	mouseSubscription = EVENT_HANDLER.subscribe(EventLambda(MouseEvent) {
		if (event.state == PRESSED) {
			if (!activeButtons[event.button])
				pressedButtons[event.button] = event.clicks;
			activeButtons[event.button] = true;
		} else if (event.state == RELEASED) {
			releasedButtons[event.button] = true;
			activeButtons[event.button] = false;
		} 
		
		//if (event.state != UNCHANGED) {
		nextMousePosition = vec2(event.position);
		//}
	});
}

void InputHandler::grabMouse(bool grabbed) {
	mouseGrabbed = grabbed;

	if (grabbed) {
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		SDL_ShowCursor(SDL_ENABLE);
	}
}

bool InputHandler::isMouseGrabbed() {
	return mouseGrabbed;
}

void InputHandler::reset(double delta) {
	if (keyPressed(KEY_ESCAPE)) {
		grabMouse(!mouseGrabbed);
	}

	for (int i = 0; i < KEYBOARD_SIZE; i++) {
		pressedKeys[i] = false;
		releasedKeys[i] = false;
	}

	for (int i = 0; i < MOUSE_SIZE; i++) {
		pressedButtons[i] = 0;
		releasedButtons[i] = false;
	}

	previousMousePosition.push_back(std::make_pair(Time::now(), nextMousePosition));

	// remove mouse positions that are too far into the past.
	while (!previousMousePosition.empty()) {
		auto it = previousMousePosition.begin();
		double elapsed = Time::time_cast<Time::time_unit, Time::seconds, double>(Time::now() - it->first);

		if (elapsed > (1.0 / 20.0)) { // smooth mouse position over the last 30th of a second
			previousMousePosition.erase(it);
		} else {
			break;
		}
	}

	currMousePosition = fvec2();
	for (int i = 0; i < previousMousePosition.size(); i++) {
		currMousePosition += previousMousePosition[i].second;
	}
	currMousePosition /= previousMousePosition.size();

	if (mouseGrabbed) {
		int32 w, h;
		Application::getWindowSize(&w, &h);
		Application::setMousePosition(w / 2, h / 2);
		mouseVelocity = (currMousePosition - vec2(w / 2, h / 2));
	} else {
		mouseVelocity = (currMousePosition - lastMousePosition);

		//if (!previousMousePosition.empty()) {
		//	auto a = previousMousePosition[0];
		//	auto b = previousMousePosition[previousMousePosition.size() - 1];
		//	double elapsed = Time::time_cast<Time::time_unit, Time::seconds, double>(b.first - a.first);
		//
		//	mouseVelocity = vec2(b.second - a.second) / float(elapsed);
		//}
	}
	lastMousePosition = currMousePosition;
}

void InputHandler::update() {
}

bool InputHandler::keyPressed(int32 key) {
	return pressedKeys[key];
}

bool InputHandler::keyDown(int32 key) {
	return activeKeys[key];
}

bool InputHandler::keyReleased(int32 key) {
	return releasedKeys[key];
}

bool InputHandler::mouseButtonPressed(int32 button, uint8 count, bool once) {
	return count == 0 ? pressedButtons[button] > 0 : once ? pressedButtons[button] == count : pressedButtons[button] >= count;
}

bool InputHandler::mouseButtonDown(int32 button) {
	return activeButtons[button];
}

bool InputHandler::mouseButtonReleased(int32 button) {
	return releasedButtons[button];
}

bool InputHandler::mouseButtonDoubleClicked(int32 button) {
	return doubleClickedButtons[button];
}

fvec2 InputHandler::getMousePosition() const {
	return currMousePosition;
}

fvec2 InputHandler::getMouseVelocity() const {
	return mouseVelocity;
}