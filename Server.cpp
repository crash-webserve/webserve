#include "Server.hpp"
#include "Socket.hpp"

//  Default constructor of Server.
//  - Parameters(None)
Server::Server() 
: _portNumber(0),
_serverName(""),
_sock(nullptr)
{
    _sinAddress.s_addr = 0;
}; 

//  Constructor of Server.
//  - Parameters
//      ipAddress: The c style string of ip address of server
//      portNumber: The port number.
//      serverName: The server name.
Server::Server(const char* ipAddress, short portNumber, const std::string& serverName)
: _portNumber(portNumber), _serverName(serverName) {
    inet_pton(AF_INET, ipAddress, &this->_sinAddress);
    _sock = nullptr;
};

//  Process request from client.
//  - Parameters
//      clientSocket: The socket of client requesting process.
//      kqueueFD: The kqueue fd is where to add write event for response.
void Server::process(Socket& clientSocket, int kqueueFD) const {
    clientSocket.addKevent(kqueueFD, EVFILT_WRITE, NULL);
}
