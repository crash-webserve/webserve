#ifndef CONNECTION_HPP_
#define CONNECTION_HPP_

#include <string>
#include <sstream>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <exception>
#include "Log.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "EventContext.hpp"

#define TCP_MTU 1500

typedef unsigned short port_t;

//  General coonection handler, from generation communication.
//   - TODO
//      Connection should handle the receiving and transmiting without malfunction.
//      소켓이 수신 결과를 Request객체로 저장할 수 있어야 함.
//      소켓이 Response객체를 이용해 송신을 처리할 수 있어야 함.
//   - Member Variables
//      _client
//      _ident
//      _addr
//      _hostPort
//      _request
//   - Methods
class Connection {
public:
    Connection(port_t port);
    ~Connection();

    Connection* acceptClient();
    ReturnCaseOfRecv receive();
    void transmit();
    static void addKevent(int filter, EventContext* context);
    static void addKeventOneshot(EventContext* context);
    static void removeKevent(int filter, EventContext* context);
    void dispose();

    static const int getKqueue() { return _kqueue; };
    bool isclient() { return this->_client; };
    int getIdent() { return this->_ident; };
    std::string getAddr() { return this->_addr; };
    port_t getPort() { return this->_hostPort; };
    std::string getPort(std::string string);
    const Response& getResponse() const { return this->_response; };
    const Request& getRequest() const { return this->_request; };
    bool isClosed() { return this->_closed; };
    void clearResponseMessage();
    void appendResponseMessage(const std::string& message);

    std::string makeHeaderField(unsigned short fieldName);
    std::string makeDateHeaderField();

private:
    static const int _kqueue;
    bool _client;
    int _ident;
    port_t _hostPort;
    std::string _addr;
    Request _request;
    Response _response;
    bool _readEventTriggered;
    bool _writeEventTriggered;
    bool _closed;

    Connection(int ident, std::string addr, port_t port);

    void newSocket();
    void bindSocket();
    void listenSocket();

    typedef unsigned char Byte;
    typedef std::vector<Byte> ByteVector;
    // typedef ByteVector::iterator ByteVectorIter;
};

//  Clear response message.
//  - Parameters(None)
//  - Return(None)
inline void Connection::clearResponseMessage() {
    this->_response.clearMessage();
}

//  Append message to response.
//  - Parameters message: message to append.
//  - Return(None)
inline void Connection::appendResponseMessage(const std::string& message) {
    this->_response.appendMessage(message);
}

#endif  // CONNECTION_HPP_
