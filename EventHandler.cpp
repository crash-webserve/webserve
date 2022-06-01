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

void EventHandler::addUserEvent(EventContext* context) {
	struct kevent ev;

    EV_SET(&ev, context->getIdent(), EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, context);
    if (kevent(_kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("AddEvent(Oneshot flagged) Failed.");
}

int EventHandler::checkEvent(struct kevent* eventlist) {
	return kevent(this->_kqueue, NULL, 0, eventlist, _maxEvent, NULL);
}
