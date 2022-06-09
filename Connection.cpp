#include "Connection.hpp"

// Constructor of Connection class ( no default constructor )
// Generates a Connection instance for servers.
//  - Parameters
//      - port: Port number to open
Connection::Connection(port_t port, EventHandler& evHandler)
: _client(false)
, _hostPort(port)
, _eventHandler(evHandler)
, _targetVirtualServer(NULL) {
    this->newSocket();
    this->bindSocket();
    this->listenSocket();
    Log::verbose("New Server Connection: socket[%d] port[%d]", _ident, _hostPort);
}

// Constructor of Connection class
// Generates a Connection instance for clients.
//  - Parameters
//      - ident: Socket FD which is delivered by accept
//      - addr: Address to the client
//      - port: Port number to open
Connection::Connection(int ident, std::string addr, port_t port, EventHandler& evHandler)
: _client(true)
, _ident(ident)
, _hostPort(port)
, _addr(addr)
, _eventHandler(evHandler)
, _closed(false)
, _targetVirtualServer(NULL) {
    Log::verbose("New Client Connection: socket[%d]", _ident);
}

// Destructor of the Socket class
// Closes opened socket file descriptor.
Connection::~Connection() {
    Log::verbose("Connection instance destructor has been called: [%d]", _ident);
    close(this->_ident);
}

// Used with accept(), creates a new Connection instance by the information of accepted client.
//  - Return
//      new Connection instance
Connection* Connection::acceptClient() {
    sockaddr_in     remoteaddr;
    socklen_t       remoteaddrSize = sizeof(remoteaddr);
    int clientfd = accept(this->_ident, reinterpret_cast<sockaddr*>(&remoteaddr), &remoteaddrSize);
    std::string     addr;
    port_t  port;

    if (clientfd < 0) {
        throw std::runtime_error("accept() Failed");
        return NULL;
    }
    addr = inet_ntoa(remoteaddr.sin_addr);
    port = ntohs(remoteaddr.sin_port);
    Log::verbose("Connected from client[%s:%d]", addr.c_str(), port);
    if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl Failed");
    return new Connection(clientfd, addr, this->_hostPort, _eventHandler);
}

// The way how Connection class handles receive event.
//  - Return
//      Result of receiving process.
EventContext::EventResult Connection::receive() {
    ReturnCaseOfRecv result = this->_request.receive(this->_ident);

	switch (result) {
	case RCRECV_ERROR:
		Log::debug("Error has been occured while recieving from [%d].", this->_ident);
	case RCRECV_ZERO:
		this->dispose();
		return EventContext::ER_Remove;
	case RCRECV_SOME:
		break;
	case RCRECV_PARSING_FINISH:
		return this->passParsedRequest();
	}
	return EventContext::ER_Continue;
}

//  Send response message to client.
//  - Parameters(None)
//  - Return(None)
EventContext::EventResult Connection::transmit() {
    ReturnCaseOfSend result = this->_response.sendResponseMessage(this->_ident);

    switch (result) {
	case RCSEND_ERROR:
		Log::debug("Error has been occured while Sending to [%d].", this->_ident);
	case RCSEND_ALL:
		return EventContext::ER_Remove;
	case RCSEND_SOME:
		break;
	default:
		assert(false);
		break;
    }
	return EventContext::ER_Continue;
}

// Clean-up process to destroy the Socket instance.
// mark close attribute, and remove all kevents enrolled.
//  - Return(none)
void Connection::dispose() {
    if (_closed == true)
        return;
    _closed = true;
    Log::verbose("Socket instance closing. [%d]", this->_ident);
	_eventHandler.addUserEvent(
		this->_ident,
        EventContext::EV_DisposeConn,
        NULL
	);
}

EventContext::EventResult Connection::handleCGIParamBody(int PipeToCGI) {
	std::string body = this->getRequest().getBody();
	size_t leftSize = body.length();
	size_t offset = 0;
    ssize_t writeResult = write(PipeToCGI, body.c_str() + offset, BUF_SIZE - 1);

    switch (writeResult) {
    case -1:
        Log::warning("CGI body pass failed.");
    case 0:
		close(PipeToCGI);
        return EventContext::ER_Remove;
    default:
		leftSize -= writeResult;
		offset += writeResult;
        return EventContext::ER_Continue;
    }
}

EventContext::EventResult Connection::handleCGIResponse(int PipeFromCGI) {
    char buffer[BUF_SIZE];
    ssize_t result = read(PipeFromCGI, buffer, BUF_SIZE - 1);

    switch (result) {
    case 0:
        this->_response.processCGIResponse();
        this->_eventHandler.addEvent(
            EVFILT_WRITE,
            this->_ident,
            EventContext::EV_Response,
            this
        );
    case -1:
        Log::debug("CGI pipe has been broken.");
        return EventContext::ER_Remove;
    default:
        buffer[result] = '\0';
        this->appendResponseMessage(buffer);
        return EventContext::ER_Continue;
    }
}

void Connection::addKevent(int filter, int fd, EventContext::EventType type, void* data) {
    this->_eventHandler.addEvent(filter, fd, type, data);
}

// Creates new Connection and set for the attribute.
//  - Return(none)
void Connection::newSocket() {
    int     newConnection = socket(PF_INET, SOCK_STREAM, 0);
    int     enable = 1;

    if (0 > newConnection) {
        throw Connection::MAKESOCKETFAIL();
    }
    if (0 > setsockopt(newConnection, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        throw Connection::SETUPSOCKETOPTFAIL();
    }
    // Log::verbose("Connection ( %d ) has been setted to Reusable.", newConnection);
    this->_ident = newConnection;
}

// Set-up addr_in structure to bind the socket.
//  - Return(none)
static void setAddrStruct(int port, sockaddr_in& addr_in) {
    std::memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = PF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = INADDR_ANY;
    // Log::verbose("Connectionadd struct has been setted");
}

// Bind socket to the designated port.
//  - Return(none)
void Connection::bindSocket() {
    sockaddr*   addr;
    sockaddr_in addr_in;
    setAddrStruct(this->_hostPort, addr_in);
    addr = reinterpret_cast<sockaddr*>(&addr_in);
    if (0 > bind(this->_ident, addr, sizeof(*addr))) {
        throw Connection::BINDSOCKETERROR();
    }
}

// Listen to the socket for incoming messages.
//  - Return(none)
void Connection::listenSocket() {
    if (0 > listen(_ident, 10)) {
        throw Connection::LISTENSOCKETERROR();
    }
    if (fcntl(this->_ident, F_SETFL, O_NONBLOCK) == -1)
        throw Connection::LISTENSOCKETERROR();
}

EventContext::EventResult Connection::passParsedRequest() {
	_eventHandler.addUserEvent(
		this->_ident,
        EventContext::EV_SetVirtualServer,
        this
	);
	return EventContext::ER_Done;
}

void Connection::parseCGIurl(std::string const &targetResourceURI, std::string const &targetExtention) {
    const std::string::size_type targetExtBeginPos = targetResourceURI.find(targetExtention);
    const std::string::size_type targetQueryBeginPos = targetResourceURI.find_first_of(std::string("?"), targetExtBeginPos);
    std::string scriptName = targetResourceURI.substr(0, targetExtBeginPos + targetExtention.size());
    std::string pathInfo = targetResourceURI.substr(targetExtBeginPos + targetExtention.size(), targetQueryBeginPos - (targetExtBeginPos + targetExtention.size()));
    std::string queryString = targetResourceURI.substr(targetQueryBeginPos + 1);;

    this->_request.updateParsedTarget(scriptName);
    this->_request.updateParsedTarget(pathInfo);
    this->_request.updateParsedTarget(queryString);
}