#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "Socket.hpp"
#include "Request.hpp"

//  TODO Add member variable from server config.
//  Server is the entity processing request from client.
//  - Member variables
//      _sinAddress: The ip address of server.
//      _portNumber: The port number of server.
//      _serverName: The server name.
//
//      _sock: The socket to accept client for this server.
class Server {
public:
    Server();
    Server(const char* ipAddress, short portNumber, const std::string& _serverName);

    struct in_addr getAddrStruct() const { return this->_sinAddress; }
    in_port_t getPortNumber() const { return this->_portNumber; }
    std::string getServerName() const { return this->_serverName; }
    void setAddrStruct(in_addr_t ipAddress) { this->_sinAddress.s_addr = ipAddress; }
    void setPortNumber(in_port_t portNumber) { this->_portNumber = portNumber; }
    void setServerName(std::string serverName) { this->_serverName = serverName; }
    
    void setSock(Socket* sock) { this->_sock = sock; };

    void process(Socket& clientSocket, int kqueueFD) const;

private:
    struct in_addr _sinAddress;
    in_port_t _portNumber;
    std::string _serverName;

    Socket* _sock;
};

#endif  // SERVER_HPP_
