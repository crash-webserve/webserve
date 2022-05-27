#ifndef EVENTHANDLER_HPP_
#define EVENTHANDLER_HPP_

#include <sys/event.h>

class EventHandler {
public:
	static void addEvent(int kqueue, int filter, void* udata);
	static void addEventOneshot(int kqueue, void* udata);
	static void removeEvent(int kqueue, int filter, void* udata);

private:
	int	_kqueue;
};

void EventHandler::addEvent(int fd, int filter, void* udata) {
	struct kevent ev;

	
}

#endif