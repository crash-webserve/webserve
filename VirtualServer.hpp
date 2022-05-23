#ifndef VIRTUALSERVER_HPP_
#define VIRTUALSERVER_HPP_

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "Location.hpp"
#include "Connection.hpp"
#include "Request.hpp"

typedef unsigned short port_t;

//  TODO Add member variable from server config.
//  VirtualServer is the entity processing request from client.
//  - Member variables
//      _portNumber: The port number of server.
//      _name: The name of server.
//      _clientMaxBodySize: The limit of body size in request messsage.
//      _location: The location directive data for VirtualServer.
//
//      _others: Variable for additional data.
//          std::string _defaultErrorPagePath: The path used to set error pages.
//
//      _connection: The connection to accept client for this server.
class VirtualServer {
public:
    VirtualServer();
    VirtualServer(short portNumber, const std::string& name);

    port_t getPortNumber() const { return this->_portNumber; }
    std::string getServerName() const { return this->_name; }
    void setPortNumber(port_t portNumber) { this->_portNumber = portNumber; }
    void setServerName(std::string serverName) { this->_name = serverName; }

    void setConnection(Connection* connection) { this->_connection = connection; };

    void process(Connection& clientConnection, int kqueueFD) const;

private:
    port_t _portNumber;
    std::string _name;
    std::size_t _clientMaxBodySize;
    std::vector<Location*> _location;

    std::map<std::string, std::string> _others;

    Connection* _connection;
};

#endif  // VIRTUALSERVER_HPP_
