#ifndef EVENTCONTEXT_HPP_
#define EVENTCONTEXT_HPP_

class Connection;
class Response;

class EventContext {
public:
	enum ContextType {
		CONNECTION,
		REQUEST,
		RESPONSE,
		CGIResponse,
	};
	enum EventResult {
		ER_Done,
		ER_Continue,
		ER_NA,		
	};

	EventContext(int fd, ContextType type, void* data);
	~EventContext();

	int getIdent() { return _eventIdent; };
	ContextType getCallerType() { return _callerType; };
	void* getData() { return _data; };
	Connection* getDataAsConnection() { return (Connection*)_data; };
	Response* getDataAsResponse() { return (Response*)_data; };

	EventResult driveThisEvent(int filter);

private:
	int _eventIdent;
	ContextType	_callerType;
	void* _data;
};

EventContext::EventContext(int fd, ContextType type, void* data)
: _eventIdent(fd)
, _callerType(type)
, _data(data) {
}

EventContext::EventResult EventContext::driveThisEvent(int filter) {
	switch (_callerType) {
	case EventContext::CONNECTION:
		if (filter != EVFILT_READ)
			return ER_NA;
		((FTServer*)_data)->acceptConnection(_eventIdent);
		return ER_Continue;
	case EventContext::REQUEST:
		if (filter != EVFILT_READ)
			return ER_NA;
		return ((Request*)_data)->receive(_eventIdent);
	case EventContext::RESPONSE:
		if (filter != EVFILT_WRITE)
			return ER_NA;
		return ((Response*)_data)->sendResponseMessage(_eventIdent);
	case EventContext::CGIResponse:
	}
}



#endif