#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "Location.hpp"
#include "Socket.hpp"
#include "Request.hpp"

typedef unsigned short port_t;

//  TODO Add member variable from server config.
//  Server is the entity processing request from client.
//  - Member variables
//      _portNumber: The port number of server.
//      _name: The name of server.
//      _clientMaxBodySize: The limit of body size in request messsage.
//      _location: The location directive data for Server.
//
//      _others: Variable for additional data.
//          std::string _defaultErrorPagePath: The path used to set error pages.
//
//      _sock: The socket to accept client for this server.
class Server {
public:
    Server(short portNumber, const std::string& name);

    in_port_t getPortNumber() const { return this->_portNumber; };
    void setSock(Socket* sock) { this->_sock = sock; };

    void process(Socket& clientSocket, int kqueueFD) const;

private:
    port_t _portNumber;
    std::string _name;
    std::size_t _clientMaxBodySize;
    std::vector<Location*> _location;

    std::map<std::string, std::string> others;

    Socket* _sock;
};

#endif  // SERVER_HPP_
