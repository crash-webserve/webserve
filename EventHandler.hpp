#ifndef EVENTHANDLER_HPP_
#define EVENTHANDLER_HPP_

#include <unistd.h>
#include <sys/event.h>
#include <exception>
#include "Log.hpp"
#include "EventContext.hpp"

class Connection;
class Response;

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

	void addEvent(int filter, EventContext* context);
	void removeEvent(int filter, EventContext* context);
	void addEventCloseConnection(EventContext* context);
	int checkEvent(struct kevent* eventlist);
	int runEachEvent(struct kevent event);

private:
	const int _kqueue;
	const int _maxEvent;

	enum { MaxEventNumber = 20 };
};

EventHandler::EventHandler()
: _kqueue(kqueue())
, _maxEvent(MaxEventNumber) {
	if (_kqueue < 0)
		throw std::runtime_error("Cannot create EventHandler.");
}

EventHandler::~EventHandler() {
	close(_kqueue);
}

void EventHandler::addEvent(int filter, EventContext* context) {
	struct kevent ev;

    EV_SET(&ev, context->getIdent(), filter, EV_ADD | EV_ENABLE, 0, 0, context);
    if (kevent(_kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("AddEvent Failed.");
}

void EventHandler::removeEvent(int filter, EventContext* context) {
	struct kevent ev;

    EV_SET(&ev, context->getIdent(), filter, EV_DELETE, 0, 0, context);
    if (kevent(_kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("AddEvent Failed.");
	delete context;
}

void EventHandler::addEventCloseConnection(EventContext* context) {
	struct kevent ev;

    EV_SET(&ev, context->getIdent(), EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, context);
    if (kevent(_kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("AddEvent(Oneshot flagged) Failed.");
}

int EventHandler::checkEvent(struct kevent* eventlist) {
	kevent(this->_kqueue, NULL, 0, eventlist, _maxEvent, NULL);
}

int EventHandler::runEachEvent(struct kevent event) {
    EventContext* context = (EventContext*)event.udata;
	int filter = event.filter;
	int eventResult;

	eventResult = context->driveThisEvent(filter);
	if (eventResult == EventContext::ER_Done)
		removeEvent(filter, context);
}



#endif