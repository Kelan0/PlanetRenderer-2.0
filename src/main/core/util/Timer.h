#pragma once

#include "core/Core.h"
#include "core/event/Event.h"
#include "core/util/Container.h"

class Subscription;
class TimerEvent;
class Timer;


class TimerEvent : public Event {
private:
	Timer* timer;

public:
	TimerEvent(Timer* timer) :
		timer(timer) {}

	double getPeriod() const;

	double getDelta() const;

	double getRuntime() const;

	uint64 getStartTime() const;

	Timer* getTimer() const;
};

class Timer
{
	using TimerCallback = typename std::function<void(TimerEvent, Subscription*)>;

private:
	Subscription* subscription;
	TimerCallback callback;
	double period;
	double delta;
	double totalRuntime;
	double localRuntime;
	double timeout;
	bool running;
	bool waiting;

	double adaptRate;
	double avgOvershoot;
	uint64 lastTime;
	uint64 startTime;

	ContainerMap varMap;

	void updateInterval(TimerEvent event, Subscription* subscription);

	void startAfter(double timeout);

public:
	Timer(TimerCallback callback, double period, double timeout = 0.0);
	~Timer();

	Timer* start();

	Timer* stop();

	Timer* reset();

	Timer* wait(double timeout);

	template<typename T>
	Timer* storeVar(std::string id, T obj) {
		varMap.put<T>(id, obj);
		return this;
	}

	template<typename T>
	T getVar(std::string id, T defaultVal) {
		return varMap.get(id, defaultVal);
	}

	double getPeriod() const;

	double getDelta() const;

	double getTotalRuntime() const;

	double getLocalRuntime() const;

	uint64 getStartTime() const;

	bool isRunning() const;

	bool isWaiting() const;

	static Timer* setTimer(TimerCallback callback, double period, double timeout = -1.0);

	static Timer* setTimeout(TimerCallback callback, double timeout);
};

