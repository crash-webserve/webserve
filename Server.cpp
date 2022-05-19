#include "Server.hpp"
#include "Socket.hpp"

//  Constructor of Server.
//  - Parameters
//      portNumber: The port number.
//      serverName: The server name.
Server::Server(short portNumber, const std::string& name)
: _portNumber(portNumber), _name(name) { }

//  Process request from client.
//  - Parameters
//      clientSocket: The socket of client requesting process.
//      kqueueFD: The kqueue fd is where to add write event for response.
void Server::process(Socket& clientSocket, int kqueueFD) const {
    clientSocket.setResponse("The random response.\r\n");
    clientSocket.addKevent(kqueueFD, EVFILT_WRITE, NULL);
}
