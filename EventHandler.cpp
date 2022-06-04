#include "EventHandler.hpp"

EventHandler::EventHandler()
: _kqueue(kqueue())
, _maxEvent(MaxEventNumber) {
	if (_kqueue < 0)
		throw std::runtime_error("Cannot create EventHandler.");
}

EventHandler::~EventHandler() {
	close(_kqueue);
}

// Add new event on Kqueue
//  - Parameters
//      kqueue: FD number of Kqueue
//      filter: filter value for Kevent
//      udata: user data (optional)
//  - Return(none)
void EventHandler::addEvent(int filter, int fd, EventContext::EventType type, void* data) {
	struct kevent ev;
	EventContext* context = new EventContext(fd, type ,data);

    EV_SET(&ev, context->getIdent(), filter, EV_ADD | EV_ENABLE, 0, 0, context);
    if (kevent(_kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("AddEvent Failed.");
}

// Remove existing event on Kqueue
//  - Parameters
//      kqueue: FD number of Kqueue
//      filter: filter value for Kevent to remove
//      udata: user data (optional)
//  - Return(none)
void EventHandler::removeEvent(int filter, EventContext* context) {
	struct kevent ev;

    EV_SET(&ev, context->getIdent(), filter, EV_DELETE, 0, 0, context);
    if (kevent(_kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("AddEvent Failed.");
	delete context;
}

// Add custom event on Kqueue (triggered just for 1 time)
//  - Parameters
//      context: EventContext for event
//  - Return(none)
void EventHandler::addUserEvent(int fd, EventContext::EventType type, void* data) {
	struct kevent ev;
	EventContext* context = new EventContext(fd, type ,data);

    EV_SET(&ev, context->getIdent(), EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, context);
    if (kevent(_kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("AddEvent(Oneshot flagged) Failed.");
}

int EventHandler::checkEvent(struct kevent* eventlist) {
	return kevent(this->_kqueue, NULL, 0, eventlist, _maxEvent, NULL);
}
