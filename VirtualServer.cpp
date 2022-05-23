#include "VirtualServer.hpp"
#include "Connection.hpp"

//  Default constructor of VirtualServer.
//  - Parameters(None)
VirtualServer::VirtualServer() 
: _portNumber(0),
_name(""),
_connection(nullptr)
{
}; 

//  Constructor of VirtualServer.
//  - Parameters
//      portNumber: The port number.
//      serverName: The server name.
VirtualServer::VirtualServer(short portNumber, const std::string& name)
: _portNumber(portNumber), _name(name) {
    _connection = nullptr;
}

//  Process request from client.
//  - Parameters
//      clientConnection: The connection of client requesting process.
//      kqueueFD: The kqueue fd is where to add write event for response.
void VirtualServer::process(Connection& clientConnection, int kqueueFD) const {
    clientConnection.setResponse(SAMPLE_RESPONSE);
    clientConnection.addKevent(kqueueFD, EVFILT_WRITE, NULL);
}
