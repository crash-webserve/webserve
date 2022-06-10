#ifndef CONNECTION_HPP_
#define CONNECTION_HPP_

#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "Log.hpp"
#include "EventHandler.hpp"
#include "VirtualServer.hpp"
#include "Request.hpp"
#include "Response.hpp"

#define TCP_MTU 1500

class VirtualServer;

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
//      _port
//      _request: store request message and parse it.
//      -response: store response message and send it to client.
//
//      _targetVirtualServer: the target to process request.
//   - Methods
class Connection {
public:
    Connection(port_t port, EventHandler& evHandler);
    ~Connection();

    bool isclient() { return this->_client; };
    int getIdent() { return this->_ident; };
    std::string getAddr() { return this->_addr; };
    port_t getPort() { return this->_hostPort; };
    const Request& getRequest() const { return this->_request; };
    const Response& getResponse() const { return this->_response; };
    bool isClosed() { return this->_closed; };
    VirtualServer* getTargetVirtualServer() { return this->_targetVirtualServer; };
    void setTargetVirtualServer(VirtualServer* targetVirtualServer) { this->_targetVirtualServer = targetVirtualServer; };

    Connection* acceptClient();
    EventContext::EventResult receive();
    EventContext::EventResult transmit();
    void dispose();
    void clearRequestMessage();
    void resetRequestStatus() { this->_request.resetStatus(); };
    void clearResponseMessage();
    void appendResponseMessage(const std::string& message);
    EventContext::EventResult handleCGIResponse(int CGIPipeOut);

    class MAKESOCKETFAIL: public std::exception {
    public:
        virtual const char* what() const throw() {
            return "socket() fail error";
        }
    };
    class SETUPSOCKETOPTFAIL: public std::exception {
    public:
        virtual const char* what() const throw() {
            return "setsocketopt() faile error";
        }
    };
    class BINDSOCKETERROR: public std::exception {
    public:
        virtual const char* what() const throw() {
            return "bind() fail error";
        }
    };
    class LISTENSOCKETERROR: public std::exception {
    public:
        virtual const char* what() const throw() {
            return "listen() fail error!!";
        }
    };

    void initResponseBodyBySize(std::string::size_type size) { this->_response.initBodyBySize(size); };
    void memcpyResponseMessage(char* buf, ssize_t size) { this->_response.memcpyMessage(buf, size); };
    bool isResponseReadAllFile() { return this->_response.isReadAllFile(); };

private:
    bool _client;
    int _ident;
    port_t _hostPort;
    std::string _addr;
    Request _request;
    Response _response;
	EventHandler& _eventHandler;
//    int _readEventTriggered;
//    int _writeEventTriggered;
    bool _closed;

    VirtualServer* _targetVirtualServer;

    Connection(int ident, std::string addr, port_t port, EventHandler& evHandler);

    void newSocket();
    void bindSocket();
    void listenSocket();
    EventContext::EventResult passParsedRequest();
};

//  Clear request message.
//  - Parameters(None)
//  - Return(None)
inline void Connection::clearRequestMessage() {
    this->_request.clearMessage();
}

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
