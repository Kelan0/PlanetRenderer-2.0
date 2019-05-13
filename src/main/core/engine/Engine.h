#pragma once

#include "core/Core.h"

class Engine
{
public:
	Engine();
	~Engine();

	void updateTickDeltaHistory(double dt);

	void updateFrameDeltaHistory(double dt);

	double getCurrentTickDelta(bool smoothed = false);

	double getCurrentFrameDelta(bool smoothed = false);

	void update(double dt);

	void render(double partialTicks, double dt);
};

