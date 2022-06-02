#include "Connection.hpp"

// Constructor of Connection class ( no default constructor )
// Generates a Connection instance for servers.
//  - Parameters
//      - port: Port number to open
Connection::Connection(port_t port, EventHandler& evHandler)
: _client(false)
, _hostPort(port)
, _eventHandler(evHandler)
, _readEventTriggered(-1)
, _writeEventTriggered(-1) {
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
, _addr(addr)
, _hostPort(port)
, _eventHandler(evHandler)
, _readEventTriggered(-1)
, _writeEventTriggered(-1)
, _closed(false) {
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
    struct kevent   ev;
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

// // The way how Connection class handles transmit event.
// //  - Return(none)
// void Connection::transmit() {
//     int sendResult = 0;

//     sendResult = send(this->_ident, _response.c_str(), _response.length(), 0);
//     _response = _response.substr(sendResult, -1);
//     if (sendResult == 0 || _response.length() == 0) {
//         this->removeKevent(_writeEventTriggered, EVFILT_WRITE, 0);

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
		new EventContext(this->_ident, EventContext::EV_DisposeConn, NULL)
	);
}

// Creates new Connection and set for the attribute.
//  - Return(none)
void Connection::newSocket() {
    int     newConnection = socket(PF_INET, SOCK_STREAM, 0);
    int     enable = 1;

    if (0 > newConnection) {
        throw;
    }
    // Log::verbose("New Server Connection ( %d )", newConnection);
    if (0 > setsockopt(newConnection, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        throw;
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
        throw; // TODO
    }
    // Log::verbose("Connection ( %d ) bind succeed.", this->_ident);
}

// Listen to the socket for incoming messages.
//  - Return(none)
void Connection::listenSocket() {
    if (0 > listen(_ident, 10)) {
        throw;
    }
    // Log::verbose("Listening from Connection ( %d ), Port ( %d ).", this->_ident, this->_hostPort);
}

EventContext::EventResult Connection::passParsedRequest() {
	_eventHandler.addUserEvent(
		new EventContext(this->_ident, EventContext::EV_SetVirtualServer, this)
	);
	return EventContext::ER_Done;
}
