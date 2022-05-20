#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include "Log.hpp"
#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "Request.hpp"
#include "Response.hpp"

#define TCP_MTU 1500

//  General socket handler, from generation communication.
//   - TODO
//      Socket should handle the receiving and transmiting without malfunction.
//      소켓이 수신 결과를 Request객체로 저장할 수 있어야 함.
//      소켓이 Response객체를 이용해 송신을 처리할 수 있어야 함.
//   - Member Variables
//      _client
//      _ident
//      _addr
//      _port
//      _request
//   - Methods
class Socket {
public:
    Socket(int port);

    Socket* acceptClient();
    void receive();
    void transmit();
    void addKevent(int kqueue, int filter, void* udata);

    bool isclient() { return this->_client; };
    int getIdent() { return this->_ident; };
    std::string getAddr() { return this->_addr; };
    int getPort() { return this->_port; };
    const Request& getRequest() const { return this->_request; };
    void addReceivedLine(const std::string& line) { this->_request.appendMessage(line); };
    void setResponse();

private:
    bool _client;
    int _ident;
    std::string _addr;
    int _port;
    Request _request;
    Response _response;

    Socket(int ident, std::string addr, int port);
  
    void setNewSocket();
    void bindThisSocket();
    void listenThisSocket();
};

#endif
