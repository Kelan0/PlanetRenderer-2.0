#pragma once

#include <typeindex>
#include "core/Core.h"
#include "core/util/Time.h"
#include "core/event/Event.h"

class EventHandler;

class Subscription;
class SubscriptionHandler;
class CallbackFunction;
class QueuedEvent;

template <typename EventType> class SubscriptionImpl;
template <typename EventType> class SubscriptionHandlerImpl;
template <typename EventType> class CallbackFunctionImpl;
template <typename EventType> class QueuedEventImpl;

#define EventLambda(eventType) (std::function<void(##eventType, Subscription*)>) [&](##eventType event, Subscription* subscription)

#define _Handler SubscriptionHandlerImpl<EventType>
#define _Subscription SubscriptionImpl<EventType>
#define _CallbackType CallbackFunctionImpl<EventType>
#define _QueuedEvent QueuedEventImpl<EventType>
#define _FunctionType std::function<void(EventType, Subscription*)>
#define _CallbackList std::vector<_CallbackType*>
#define _SubscriptionList std::vector<_Subscription*>
#define _EventQueue std::vector<_QueuedEvent*>

//////////////////////////////// VIRTUAL INTERFACES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

class Subscription {
public:
	virtual ~Subscription() {}

	virtual double getTimeout() const = 0;

	virtual double getDuration() const = 0;

	virtual double getLastInvocation() const = 0;

	virtual void setTimeout(double timeout) = 0;

	virtual void reset() = 0;

	virtual bool isExpired() const = 0;

	virtual bool unsubscribe() = 0;
};

class SubscriptionHandler {
public:
	virtual ~SubscriptionHandler() {};

	virtual void processQueue() = 0;
};

class CallbackFunction {
public:
	virtual ~CallbackFunction() {};
};

class QueuedEvent {
public:
	virtual ~QueuedEvent() {};

	virtual bool shouldProcess() = 0;
};


//////////////////////////////// EVENT HANDLER \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


class EventHandler {

private:
	std::unordered_map<std::type_index, SubscriptionHandler*> handlerMap;

	template <typename EventType>
	_Handler* getSubscriptionHandler() {
		std::type_index typeIdx = typeid(EventType);

		auto it = this->handlerMap.find(typeIdx);

		_Handler* handler = NULL;

		if (it != this->handlerMap.end()) {
			handler = dynamic_cast<_Handler*>(it->second);
		}

		if (handler == NULL) {
			handler = new _Handler();

			this->handlerMap[typeIdx] = handler;
		}

		return handler;
	}
public:
	EventHandler():
		handlerMap() {
	}

	~EventHandler() {}

	template <typename EventType>
	Subscription* subscribe(_FunctionType& callback) {
		_Handler* handler = getSubscriptionHandler<EventType>();

		return static_cast<Subscription*>(handler->subscribe(callback));
	}

	template <typename EventType>
	Subscription* subscribe(_FunctionType const& callback) {
		_Handler* handler = getSubscriptionHandler<EventType>();

		return static_cast<Subscription*>(handler->subscribe(callback));
	}

	template <typename EventType>
	Subscription* subscribe(void(*callback)(EventType, Subscription*)) {
		_Handler* handler = getSubscriptionHandler<EventType>();

		return static_cast<Subscription*>(handler->subscribe(callback));
	}

	bool unsubscribe(Subscription* subscription) {
		if (subscription != NULL) {
			return subscription->unsubscribe();
		} else {
			return false;
		}
	}

	template <typename EventType>
	void fire(EventType event) {
		_Handler* handler = getSubscriptionHandler<EventType>();
		handler->fire(event);
	}

	template <typename EventType>
	void enqueue(EventType event, double timeout = 0.0) {
		_Handler* handler = getSubscriptionHandler<EventType>();
		handler->enqueue(event, timeout);
	}

	void processQueue() {
		for (auto it = this->handlerMap.begin(); it != this->handlerMap.end(); it++) {
			SubscriptionHandler* handler = it->second;

			if (handler != NULL) {
				handler->processQueue();
			}
		}
	}
};


//////////////////////////////// INTERFACE IMPLEMENTATIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


template <typename EventType>
class SubscriptionImpl : public Subscription {
	friend class _Handler;

private:
	_Handler* handler;
	_CallbackType* callback;
	uint64 start;
	uint64 lastInvocation;
	double timeout;

public:
	SubscriptionImpl(_Handler* handler, _CallbackType* callback):
		handler(handler), callback(callback), start(Time::now()), lastInvocation(-1.0), timeout(-1.0) {}

	~SubscriptionImpl() {
		this->unsubscribe();
	}

	double getTimeout() const override {
		return this->timeout;
	}

	double getDuration() const override {
		return (Time::now() - this->start) / 1000000000.0;
	}

	double getLastInvocation() const {
		return (this->lastInvocation - this->start) / 1000000000.0;
	}

	void setTimeout(double timeout) override {
		this->timeout = timeout;
	}

	void reset() override {
		this->start = Time::now();
	}

	bool isExpired() const override {
		return this->getTimeout() >= 0.0 && this->getDuration() > this->getTimeout();
	}

	bool unsubscribe() override {
		if (this->handler != NULL) {
			this->handler->unsubscribe(this);
		}

		return true;
	}
};



template <typename EventType>
class SubscriptionHandlerImpl : public SubscriptionHandler {

private:
	_SubscriptionList subscriptionList;
	_EventQueue eventQueue;
public:
	SubscriptionHandlerImpl():
		subscriptionList(), eventQueue() {}

	~SubscriptionHandlerImpl() {
		// TODO: invalidate all subscriptions that were created
	}

	_Subscription* subscribe(_FunctionType callback) {
		_Subscription* subscription = new _Subscription(this, new _CallbackType(callback));
		this->subscriptionList.push_back(subscription);

		return subscription;
	}

	void unsubscribe(_Subscription* subscription) {
		if (subscription != NULL) {
			auto it = std::find(this->subscriptionList.begin(), this->subscriptionList.end(), subscription);

			if (it != this->subscriptionList.end()) {
				this->subscriptionList.erase(it);
			}
			// delete callback; // needed??
		}
	}

	void fire(EventType event) {

		std::queue<_Subscription*> processList(std::deque<_Subscription*>(this->subscriptionList.begin(), this->subscriptionList.end()));

		while (!processList.empty()) {
			_Subscription* subscription = processList.front();
			processList.pop();

			if (subscription != NULL && !subscription->isExpired()) {
				uint64 now = Time::now();
				(*subscription->callback)(event, subscription);
				subscription->lastInvocation = now;
			}
		}
	}

	void enqueue(EventType event, double timeout = 0.0) {
		this->eventQueue.push_back(new _QueuedEvent(event, timeout));
	}

	void processQueue() override {
		std::vector<EventType> firedEvents;

		for (auto it = this->eventQueue.begin(); it != this->eventQueue.end();) {
			_QueuedEvent* event = *it;

			if (event != NULL) {
				if (event->shouldProcess()) {
					firedEvents.push_back(event->getEvent());
					it = this->eventQueue.erase(it);
				} else {
					it++;
				}
			} else {
				it = this->eventQueue.erase(it);
			}
		}

		for (auto it = firedEvents.begin(); it != firedEvents.end(); it++) {
			this->fire(*it);
		}
	}
};



template <typename EventType>
class CallbackFunctionImpl : public CallbackFunction {
private:
	_FunctionType function;
public:
	CallbackFunctionImpl(_FunctionType function) :
		function(function) {}

	void operator() (EventType event, Subscription* subscription) const {
		this->function(event, subscription);
	}
};



template <typename EventType>
class QueuedEventImpl : public QueuedEvent {
private:
	EventType event;
	double timeout;
	uint64 start;

public:
	QueuedEventImpl(EventType event, double timeout) :
		event(event), timeout(timeout), start(Time::now()) {}


	EventType getEvent() const {
		return this->event;
	}

	double getTimeout() const {
		return this->timeout;
	}

	double getDuration() const {
		return (Time::now() - start) / 1000000000.0;
	}

	bool shouldProcess() override {
		return this->getDuration() >= this->getTimeout();
	}
};

#undef _Handler
#undef _Subscription
#undef _CallbackType
#undef _QueuedEvent
#undef _FunctionType
#undef _CallbackList
#undef _SubscriptionList
#undef _EventQueue