#include "EventContext.hpp"

EventContext::EventContext(int fd, EventType type, void* data)
: _eventIdent(fd)
, _callerType(type)
, _data(data) {
}

std::string EventContext::getCallerTypeToString() {
	switch (this->_callerType) {
	case EV_Accept:
		return "EV_Accept";
	case EV_SetVirtualServer:
		return "EV_SetVirtualServer";
	case EV_DisposeConn:
		return "EV_DisposeConn";
	case EV_Request:
		return "EV_Request";
	case EV_Response:
		return "EV_Response";
	case EV_CGIResponse:
		return "EV_CGIResponse";
    case EV_SetVirtualServerErrorPage:
        return "EV_SetVirtualServerErrorPage";
    case EV_GETResponse:
        return "EV_GETResponse";
    case EV_POSTResponse:
        return "EV_POSTResponse";
	}
}
