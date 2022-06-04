#ifndef EVENTHANDLER_HPP_
#define EVENTHANDLER_HPP_

#include <unistd.h>
#include <sys/event.h>
#include <exception>
#include "Log.hpp"
#include "EventContext.hpp"

class EventHandler {
public:
	enum {
		CONNECTION,
		CGIResponse,
	};

	EventHandler();
	~EventHandler();

	const int getKqueue() { return _kqueue; };
	const int getMaxEvent() { return _maxEvent; };

	void addEvent(int filter, int fd, EventContext::EventType type, void* data);
	void removeEvent(int filter, EventContext* context);
	void addUserEvent(int fd, EventContext::EventType type, void* data);
	int checkEvent(struct kevent* eventlist);

private:
	const int _kqueue;
	const int _maxEvent;

	enum { MaxEventNumber = 20 };
};

#endif