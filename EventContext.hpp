#ifndef EVENTCONTEXT_HPP_
#define EVENTCONTEXT_HPP_

class Connection;
class Response;

class EventContext {
public:
	enum {
		CONNECTION,
		CGIResponse,
	};

	EventContext(int fd, int type, void* data);
	~EventContext();

	int getIdent() { return _eventIdent; };
	int getCallerType() { return _callerType; };
	void* getData() { return _data; };
	Connection* getDataAsConnection() { return (Connection*)_data; };
	Response* getDataAsResponse() { return (Response*)_data; };

private:
	int _eventIdent;
	int	_callerType;
	void* _data;
};

#endif