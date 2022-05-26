#include "Connection.hpp"

// Constructor of Connection class ( no default constructor )
// Generates a Connection instance for servers.
//  - Parameters
//      - port: Port number to open
Connection::Connection(int port)
: _client(false)
, _hostPort(port)
, _readEventTriggered(-1)
, _writeEventTriggered(-1) {
    this->newSocket();
    this->bindSocket();
    this->listenSocket();
}

// Constructor of Connection class
// Generates a Connection instance for clients.
//  - Parameters
//      - ident: Socket FD which is delivered by accept
//      - addr: Address to the client
//      - port: Port number to open
Connection::Connection(int ident, std::string addr, int port)
: _client(true)
, _ident(ident)
, _addr(addr)
, _hostPort(port)
, _readEventTriggered(-1)
, _writeEventTriggered(-1)
, _closed(false) {
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
    int             port;

    if (clientfd < 0) {
        throw std::runtime_error("accept() Failed");
        return NULL;
    }
    addr = inet_ntoa(remoteaddr.sin_addr);
    port = ntohs(remoteaddr.sin_port);
    Log::verbose("Connected from [%s:%d]", addr.c_str(), port);
    if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl Failed");
    return new Connection(clientfd, addr, this->_hostPort);
}

// The way how Connection class handles receive event.
//  - Return
//      Result of receiving process.
ReturnCaseOfRecv Connection::receive() {
    return this->_request.receive(this->_ident);
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
void    Connection::transmit() {
    const int returnValue = this->_response.sendResponseMessage(this->_ident);
    switch (returnValue) {
        case RCSEND_SOME:
            break;
        case RCSEND_ERROR:
            // TODO Implement behavior.
        case RCSEND_ALL:
            this->removeKevent(this->_writeEventTriggered, EVFILT_WRITE, 0);
            break;
        default:
            assert(false);
            break;
    }
}

// Add new event on Kqueue
//  - Parameters
//      kqueue: FD number of Kqueue
//      filter: filter value for Kevent
//      udata: user data (optional)
//  - Return(none)
void Connection::addKevent(int kqueue, int filter, void* udata) {
    struct kevent   ev;

    EV_SET(&ev, this->_ident, filter, EV_ADD | EV_ENABLE, 0, 0, udata);
    if (kevent(kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("kevent adding Failed.");
    if (filter == EVFILT_READ) {
        this->_readEventTriggered = kqueue;
    } else if (filter == EVFILT_WRITE) {
        this->_writeEventTriggered = kqueue;
    }
}

// Add new Oneshot event on Kqueue (triggered just for 1 time)
//  - Parameters
//      kqueue: FD number of Kqueue
//      udata: user data (optional)
//  - Return(none)
void Connection::addKeventOneshot(int kqueue, void* udata) {
    struct kevent   ev;

    Log::verbose("Adding oneshot kevent...");
    EV_SET(&ev, this->_ident, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, udata);
    if (kevent(kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("kevent (Oneshot) adding Failed.");
}

// Remove existing event on Kqueue
//  - Parameters
//      kqueue: FD number of Kqueue
//      filter: filter value for Kevent to remove
//      udata: user data (optional)
//  - Return(none)
void Connection::removeKevent(int kqueue, int filter, void* udata) {
    struct kevent   ev;

    EV_SET(&ev, this->_ident, filter, EV_DELETE, 0, 0, udata);
    if (kevent(kqueue, &ev, 1, 0, 0, 0) < 0)
        throw std::runtime_error("kevent deletion Failed.");
    if (filter == EVFILT_READ) {
        this->_readEventTriggered = -1;
    } else if (filter == EVFILT_WRITE) {
        this->_writeEventTriggered = -1;
    }
}

// Clean-up process to destroy the Socket instance.
// mark close attribute, and remove all kevents enrolled.
//  - Return(none)
void Connection::dispose() {
    int kqueue = this->_readEventTriggered;

    if (_closed == true)
        return;
    _closed = true;
    Log::verbose("Socket instance closing. [%d]", this->_ident);
    if (this->_readEventTriggered >= 0) {
        Log::verbose("Read Kevent removing.");
        this->removeKevent(this->_readEventTriggered, EVFILT_READ, 0);
    }
    this->addKeventOneshot(kqueue, 0);
}

std::string Connection::makeHeaderField(unsigned short fieldName) {
    switch (fieldName)
    {
    case HTTP::DATE:
        return makeDateHeaderField();
    }
    return ""; // TODO delete
}

// Find the current time based on GMT
//  - Parameters(None)
//  - Return
//      Current time based on GMT(std::string)
std::string Connection::makeDateHeaderField() {
    std::string weekDay[7] = {"Sun", "Mon", "Tue", "Wen", "Thu" ,"Fri" ,"Sat"};
    std::string Month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    time_t rawTime;
    struct tm *ptm;
    std::string dateStr;

    time(&rawTime);
    ptm = gmtime(&rawTime);
    dateStr = weekDay[ptm->tm_wday];
    dateStr += ", ";
    dateStr += std::to_string(ptm->tm_mday);
    dateStr += " ";
    dateStr += Month[ptm->tm_mon];
    dateStr += " ";
    dateStr += std::to_string(ptm->tm_year + 1900);
    dateStr += " ";
    if (ptm->tm_hour < 10)
        dateStr += "0";
    dateStr += std::to_string(ptm->tm_hour);
    dateStr += ":";
    if (ptm->tm_min < 10)
        dateStr += "0";
    dateStr += std::to_string(ptm->tm_min);
    dateStr += ":";
    if (ptm->tm_sec < 10)
        dateStr += "0";
    dateStr += std::to_string(ptm->tm_sec);
    dateStr += " GMT";

    return dateStr;
}

// Creates new Connection and set for the attribute.
//  - Return(none)
void Connection::newSocket() {
    int     newConnection = socket(PF_INET, SOCK_STREAM, 0);
    int     enable = 1;

    if (0 > newConnection) {
        throw;
    }
    Log::verbose("New Server Connection ( %d )", newConnection);
    if (0 > setsockopt(newConnection, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        throw;
    }
    Log::verbose("Connection ( %d ) has been setted to Reusable.", newConnection);
    this->_ident = newConnection;
}

// Set-up addr_in structure to bind the socket.
//  - Return(none)
static void setAddrStruct(int port, sockaddr_in& addr_in) {
    std::memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = PF_INET;
    addr_in.sin_port = htons(port);
    // addr_in.sin_addr.s_addr = INADDR_ANY;
    Log::verbose("Connectionadd struct has been setted");
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
    Log::verbose("Connection ( %d ) bind succeed.", socket);
}

// Listen to the socket for incoming messages.
//  - Return(none)
void Connection::listenSocket() {
    if (0 > listen(_ident, 10)) {
        throw;
    }
    Log::verbose("Listening from Connection ( %d ), Port ( %d ).", _ident);
}
