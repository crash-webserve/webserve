#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "Log.hpp"
#include "Request.hpp"

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
    ~Socket();

    Socket* acceptClient();
    void receive(std::string& line);
    void transmit();
    void addKevent(int kqueue, int filter, void* udata);
    void addKeventOneshot(int kqueue, void* udata);
    void removeKevent(int kqueue, int filter, void* udata);
    void dispose();

    bool isclient() { return this->_client; };
    int getIdent() { return this->_ident; };
    std::string getAddr() { return this->_addr; };
    int getPort() { return this->_port; };
    const Request& getRequest() const { return this->_request; };
    void addReceivedLine(const std::string& line) { this->_request.appendMessage(line); };
    bool isClosed() { return this->_closed; };
    void setResponse(const std::string& line) { this->_response = line; };

private:
    bool _client;
    int _ident;
    int _port;
    std::string _addr;
    Request _request;
    std::string _response;  // 나중에 Response객체로 대체
    int _readEventTriggered;
    int _writeEventTriggered;
    bool _closed;

    Socket(int ident, std::string addr, int port);

    void setNewSocket();
    void bindThisSocket();
    void listenThisSocket();

    typedef unsigned char Byte;
    typedef std::vector<Byte> ByteVector;
    // typedef ByteVector::iterator ByteVectorIter;
};

#endif
