#pragma once

#include "core/Core.h"
#include "core/util/Time.h"

typedef enum ButtonState {
	UNCHANGED,
	PRESSED,
	RELEASED,
} ButtonState;

struct Event {
	uint64 timestamp;

	Event() :
		timestamp(Time::now()) {}
};

struct WindowClosingEvent : public Event {};

struct WindowResizeEvent : public Event {
	bool fullscreen;
	int32 oldWidth;
	int32 oldHeight;
	int32 newWidth;
	int32 newHeight;

	WindowResizeEvent(bool fullscreen, int32 oldWidth, int32 oldHeight, int32 newWidth, int32 newHeight) :
		fullscreen(fullscreen), oldWidth(oldWidth), oldHeight(oldHeight), newWidth(newWidth), newHeight(newHeight) {}
};

struct EngineInitializationEvent : public Event {};

struct EngineStartedEvent : public Event {};

struct KeyboardEvent : public Event {

	ButtonState state;
	int32 count;
	int32 keycode;
	//int32 modifiers;

	KeyboardEvent(ButtonState type, int32 count, int32 keycode) :
		state(type), count(count), keycode(keycode) {}
};

struct MouseEvent : public Event {
	ButtonState state;
	uint8 button;
	uint8 clicks;
	ivec2 position;
	ivec2 motion;

	MouseEvent(ButtonState type, uint8 button, uint8 clicks, ivec2 position, ivec2 motion):
		state(type), button(button), clicks(clicks), position(position), motion(motion) {}
};