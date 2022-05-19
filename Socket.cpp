#include "Socket.hpp"

// Constructor of Socket class ( no default constructor )
// Generates a Socket instance for servers.
//  - Parameters
//      - port: Port number to open
Socket::Socket(int port)
: _client(false)
, _port(port)
, _readEventTriggered(-1)
, _writeEventTriggered(-1)
, _closed(false) {
    setNewSocket();
    bindThisSocket();
    listenThisSocket();
}

// Constructor of Socket class
// Generates a Socket instance for clients.
//  - Parameters
//      - ident: Socket FD which is delivered by accept
//      - addr: Address to the client
//      - port: Port number to open
Socket::Socket(int ident, std::string addr, int port)
: _client(true)
, _ident(ident)
, _addr(addr)
, _port(port)
, _readEventTriggered(-1)
, _writeEventTriggered(-1)
, _closed(false) {
}

// Destructor of the Socket class
// Closes opened socket file descriptor.
Socket::~Socket() {
    Log::Verbose("Socket instance destructor has been called: [%d]", _ident);
    close(_ident);
}

// Used with accept(), creates a new Socket instance by the information of accepted client.
//  - Return
//      new Socket instance
Socket* Socket::acceptClient() {
    sockaddr_in     remoteaddr;
    socklen_t       remoteaddrSize = sizeof(remoteaddr);
    struct kevent   ev;
    int clientfd = accept(_ident, reinterpret_cast<sockaddr*>(&remoteaddr), &remoteaddrSize);
    std::string     addr;

    if (clientfd < 0) {
        throw std::runtime_error("accept() Failed");
        return NULL;
    }
    addr = inet_ntoa(remoteaddr.sin_addr);
    Log::Verbose("Connected from [%s:%d]", addr.c_str(), remoteaddr.sin_port);
    if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl Failed");
    return new Socket(clientfd, addr, remoteaddr.sin_port);
}

// The way how Socket class handles receive event.
//  - Return(none)
void Socket::receive(std::string& line) {
    Byte buffer[TCP_MTU];
    int recvResult = 0;

    recvResult = recv(_ident, buffer, TCP_MTU, 0);
    if (recvResult <= 0) {
        dispose();
    } else {
        line.append(reinterpret_cast<char*>(buffer), recvResult);
    }
    Log::Verbose("receive works");
}

// The way how Socket class handles transmit event.
//  - Return(none)
void Socket::transmit() {
    int sendResult = 0;

    sendResult = send(this->_ident, _response.c_str(), _response.length(), 0);
    _response = _response.substr(sendResult, -1);
    if (sendResult == 0 || _response.length() == 0) {
        this->removeKevent(_writeEventTriggered, EVFILT_WRITE, 0);
    }
}

// Add new event on Kqueue
//  - Parameters
//      kqueue: FD number of Kqueue
//      filter: filter value for Kevent
//      udata: user data (optional)
//  - Return(none)
void Socket::addKevent(int kqueue, int filter, void* udata) {
    struct kevent   ev;

    EV_SET(&ev, _ident, filter, EV_ADD | EV_ENABLE, 0, 0, udata);
    if (kevent(kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("kevent adding Failed.");
    if (filter == EVFILT_READ) {
        _readEventTriggered = kqueue;
    } else if (filter == EVFILT_WRITE) {
        _writeEventTriggered = kqueue;
    }
}

// Add new Oneshot event on Kqueue (triggered just for 1 time)
//  - Parameters
//      kqueue: FD number of Kqueue
//      udata: user data (optional)
//  - Return(none)
void Socket::addKeventOneshot(int kqueue, void* udata) {
    struct kevent   ev;

    Log::Verbose("Adding oneshot kevent...");
    EV_SET(&ev, _ident, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, udata);
    if (kevent(kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("kevent (Oneshot) adding Failed.");
}

// Remove existing event on Kqueue
//  - Parameters
//      kqueue: FD number of Kqueue
//      filter: filter value for Kevent to remove
//      udata: user data (optional)
//  - Return(none)
void Socket::removeKevent(int kqueue, int filter, void* udata) {
    struct kevent   ev;

    EV_SET(&ev, _ident, filter, EV_DELETE, 0, 0, udata);
    if (kevent(kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("kevent deletion Failed.");
    if (filter == EVFILT_READ) {
        _readEventTriggered = -1;
    } else if (filter == EVFILT_WRITE) {
        _writeEventTriggered = -1;
    }
}

// Clean-up process to destroy the Socket instance.
// mark close attribute, and remove all kevents enrolled.
//  - Return(none)
void Socket::dispose() {
    int kqueue = _readEventTriggered;

    if (_closed == true)
        return;
    _closed = true;
    Log::Verbose("Socket instance closing. [%d]", this->_ident);
    if (_readEventTriggered >= 0) {
        Log::Verbose("Read Kevent removing.");
        removeKevent(_readEventTriggered, EVFILT_READ, 0);
    }
    addKeventOneshot(kqueue, 0);
}

// Creates new socket and set for the attribute.
//  - Return(none)
void Socket::setNewSocket() {
    int     newSocket = socket(PF_INET, SOCK_STREAM, 0);
    int     enable = 1;

    if (0 > newSocket) {
        throw;
    }
    Log::Verbose("New Server Socket ( %d )", newSocket);
    if (0 > setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        throw;
    }
    Log::Verbose("Socket ( %d ) has been setted to Reusable.", newSocket);
    _ident = newSocket;
}

// Set-up addr_in structure to bind the socket.
//  - Return(none)
static void setSocketAddr(int port, sockaddr_in& addr_in) {
    std::memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = PF_INET;
    addr_in.sin_port = htons(port);
    // addr_in.sin_addr.s_addr = INADDR_ANY;
    Log::Verbose("Socketadd struct has been setted");
}

// Bind socket to the designated port.
//  - Return(none)
void Socket::bindThisSocket() {
    sockaddr*   addr;
    sockaddr_in addr_in;
    setSocketAddr(_port, addr_in);
    addr = reinterpret_cast<sockaddr*>(&addr_in);
    if (0 > bind(_ident, addr, sizeof(*addr))) {
        throw;
    }
    Log::Verbose("Socket ( %d ) bind succeed.", socket);
}

// Listen to the socket for incoming messages.
//  - Return(none)
void Socket::listenThisSocket() {
    if (0 > listen(_ident, 10)) {
        throw;
    }
    Log::Verbose("Listening from Socket ( %d ), Port ( %d ).", _ident);
}
