#include "Timer.h"
#include "core/event/Event.h"
#include "core/event/EventHandler.h"
#include "core/application/Application.h"

double TimerEvent::getPeriod() const {
	return this->timer->getPeriod();
}

double TimerEvent::getDelta() const {
	return this->timer->getDelta();
}

double TimerEvent::getRuntime() const {
	return this->timer->getTotalRuntime();
}

uint64 TimerEvent::getStartTime() const {
	return this->timer->getStartTime();
}

Timer* TimerEvent::getTimer() const {
	return this->timer;
}


Timer::Timer(TimerCallback callback, double period, double timeout) :
	subscription(NULL),
	callback(callback),
	period(period),
	delta(period),
	totalRuntime(0.0),
	localRuntime(0.0),
	timeout(timeout),
	running(false),
	waiting(false),
	adaptRate(0.03),
	avgOvershoot(0.0),
	lastTime(Time::now()),
	startTime(Time::now())
{}


Timer::~Timer() {
	if (this->subscription != NULL) {
		this->subscription->unsubscribe();
	}
}

void Timer::updateInterval(TimerEvent event, Subscription* subscription) {
	if (event.getTimer() == this) {
		using namespace Time;

		double duration = subscription->getDuration(); // The time the subscription has existed (time since loop started)
		double expected = std::round(duration / period) * period; // The expected duration given perfect conditions.
		double overshoot = duration - expected; // The difference between the expected and actual duration.

		// adaptRate = 0.03 ...another arbitary value which determines how fast avgOvershoot reacts
		// to sudden changes. Lower values smoothe out one-off frame time spikes, but mean it takes
		// longer to adapt to a sustained increase in frame time.
		this->avgOvershoot = (1.0 - this->adaptRate) * this->avgOvershoot + this->adaptRate * overshoot;

		uint64 t0 = now();

		this->delta = time_cast<time_unit, seconds, double>(t0 - this->lastTime);
		this->totalRuntime = time_cast<time_unit, seconds, double>(t0 - this->startTime);
		this->localRuntime += delta;
		this->lastTime = t0;

		// call the callback function and time its execution time. This is used to adjust the time delay until
		// the next call.
		this->callback(event, subscription);
		uint64 t1 = now();
		double elapsed = time_cast<time_unit, seconds, double>(t1 - t0);

		if (this->timeout < 0.0 || this->totalRuntime < this->timeout) {
			EVENT_HANDLER.enqueue<TimerEvent>(TimerEvent(this), std::max(0.0, period - (overshoot + elapsed + avgOvershoot * 20.0)));
			// 20 is the correction parameter... kind of arbitary value which took a while to tune
			// perfectly... the higher the value, the faster the loop compensates for any deviation
			// from the expected update rate, but too high of a value causes the loop to become unstable,
			// ocellating back and forth between overcompensating and undercompensating, expenontially
			// getting worse with every update.
		} else {
			this->stop();
		}
	}
}

void Timer::startAfter(double timeout) {
	if (this->running) {
		this->stop();
	}
	this->running = true;

;	std::function<void(TimerEvent, Subscription*)> func = std::bind(&Timer::updateInterval, this, std::placeholders::_1, std::placeholders::_2);
	Subscription* subscription = EVENT_HANDLER.subscribe<TimerEvent>(func);

	EVENT_HANDLER.enqueue<TimerEvent>(TimerEvent(this), timeout); // Enqueue the first event.

	this->subscription = subscription;
}

Timer* Timer::start() {
	this->startAfter(this->period);
	return this;
}

Timer* Timer::stop() {
	this->avgOvershoot = 0.0;
	this->running = false;

	if (this->subscription != NULL) {
		this->subscription->unsubscribe();
		this->subscription = NULL;
	}

	return this;
}

Timer* Timer::reset() {
	this->totalRuntime = 0.0;
	this->localRuntime = 0.0;
	this->startTime = Time::now();
	this->lastTime = this->startTime;
	return this;
}

Timer* Timer::wait(double timeout){
	if (!this->waiting) {
		printf("Sleeping...\n");
		this->waiting = true;
		this->stop();

		// store wait timer as a class field, so that it can be cancelled, and so
		// that we can check if we are currently waiting in place of the `waiting` bool
		Timer::setTimeout([&](TimerEvent event, Subscription* subscription) {
			printf("Waking up...\n");
			this->waiting = false;
			this->start();
		}, timeout);
	}

	return this;
}

double Timer::getPeriod() const {
	return this->period;
}

double Timer::getDelta() const {
	return this->delta;
}

double Timer::getTotalRuntime() const {
	return this->totalRuntime;
}

double Timer::getLocalRuntime() const {
	return this->localRuntime;
}

uint64 Timer::getStartTime() const {
	return this->startTime;
}

bool Timer::isRunning() const {
	return this->running;
}

bool Timer::isWaiting() const {
	return this->waiting;
}

Timer* Timer::setTimer(TimerCallback callback, double period, double timeout) {
	Timer* timer = new Timer(callback, period, timeout);
	timer->reset();
	timer->start();

	return timer;
}

Timer* Timer::setTimeout(TimerCallback callback, double timeout) {
	Timer* timer = new Timer(callback, timeout, timeout);
	timer->start();

	return timer;
}
