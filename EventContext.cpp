#include "EventContext.hpp"

EventContext::EventContext(int fd, EventType type, void* data)
: _eventIdent(fd)
, _callerType(type)
, _data(data) {
}