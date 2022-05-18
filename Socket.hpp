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

class Socket {
public:
    /**
     * @brief Creates the server socket.
     * 
     */
    Socket(int port);

    Socket*         acceptClient();
    void            receive();
    void            transmit();
    void            addKevent(int kqueue, int filter, void* udata);

    bool            isclient();
    int             getIdent();
    std::string     getAddr();
    int             getPort();
    std::string     getHTTPMessage();
    const Request&  getRequest() const { return this->_request; };
    void addReceivedLine(const std::string& line) { this->_request.appendMessage(line); };
    void setResponse();

private:
    bool            _client;
    int             _ident;
    std::string     _addr;
    int             _port;
    std::string     _HTTPMessage;

    Socket(int ident, std::string addr, int port);

    void            setNewSocket();
    void            bindThisSocket();
    void            listenThisSocket();

    Request         _request;
    Response        _response;
};

#endif
