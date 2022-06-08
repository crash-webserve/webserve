#ifndef EVENTCONTEXT_HPP_
#define EVENTCONTEXT_HPP_

#include <string>

class EventContext {
public:
	enum EventType {
		EV_Accept,
		EV_SetVirtualServer,
		EV_DisposeConn,
		EV_Request,
		EV_Response,
		EV_CGIResponse,
        EV_SetVirtualServerErrorPage,
        EV_GETResponse,
        EV_POSTResponse,
	};
	enum EventResult {
		ER_Done,
		ER_Remove,
		ER_Continue,
		ER_NA,
	};

	EventContext(int fd, EventType type, void* data);

	int getIdent() { return _eventIdent; };
	EventType getCallerType() { return _callerType; };
	std::string getCallerTypeToString();
	void* getData() { return _data; };

private:
	int _eventIdent;
	EventType	_callerType;
	void* _data;
};

#endif
