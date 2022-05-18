#include "Server.hpp"
#include "Socket.hpp"

//  Constructor of Server.
//  - Parameters
//      ipAddress: The c style string of ip address of server.
//      portNumber: The port number.
//      serverName: The server name.
Server::Server(const char* ipAddress, short portNumber, const std::string& serverName)
: _portNumber(portNumber), _serverName(serverName) {
    inet_pton(AF_INET, ipAddress, &this->_sinAddress);
};

//  Process request from client.
//  - Parameters
//      clientSocket: The socket of client requesting process.
//      kqueueFD: The kqueue fd is where to add write event for response.
void Server::process(Socket& clientSocket, int kqueueFD) const {
    // TODO Process request in clientSocket, and set response._message in clientSocket.
    clientSocket.setResponse();
    clientSocket.addKevent(kqueueFD, EVFILT_WRITE, NULL);
}
