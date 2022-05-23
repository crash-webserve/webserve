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
#include "Request.hpp"
#include "Response.hpp"

#define TCP_MTU 1500
#define SAMPLE_RESPONSE "HTTP/1.1 200 OK\r\n\
Content-Length: 365\r\n\
\r\n\
<!DOCTYPE html>\r\n\
<html>\r\n\
<head>\r\n\
<title>Welcome to nginx!</title>\r\n\
<style>\r\n\
html { color-scheme: light dark; }\r\n\
body { width: 35em; margin: 0 auto;\r\n\
font-family: Tahoma, Verdana, Arial, sans-serif; }\r\n\
</style>\r\n\
</head>\r\n\
<body>\r\n\
<h1>Welcome to nginx!</h1>\r\n\
\r\n\
<p><em>Thank you for using nginx.</em></p>\r\n\
</body>\r\n\
</html>\r\n\
\r\n\
\r\n"

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
//      _request
//   - Methods
class Connection {
public:
    Connection(int port);
    ~Connection();

    Connection* acceptClient();
    ReturnCaseOfRecv receive();
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
    bool isClosed() { return this->_closed; };
    void setResponse(const std::string& line) { this->_response.appendMessage(line.c_str()); };

private:
    bool _client;
    int _ident;
    int _port;
    std::string _addr;
    Request _request;
    Response _response;
    int _readEventTriggered;
    int _writeEventTriggered;
    bool _closed;

    Connection(int ident, std::string addr, int port);

    void newSocket();
    void bindSocket();
    void listenSocket();

    typedef unsigned char Byte;
    typedef std::vector<Byte> ByteVector;
    // typedef ByteVector::iterator ByteVectorIter;
};

#endif  // CONNECTION_HPP_
