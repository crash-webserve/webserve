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

struct StatusCode {
    static const char* EMPTY;
    static const char* OK;
    static const char* CREATED;
    static const char* MOVED_PERMANENTLY;
    static const char* BAD_REQUEST;
    static const char* FORBIDDEN;
    static const char* NOT_FOUND;
    static const char* METHOD_NOT_ALLOWED;
    static const char* LENGTH_REQUIRED;
    static const char* PAYLOAD_TOO_LARGE;
    static const char* INTERNAL_SERVER_ERROR;
    static const char* HTTP_VERSION_NOT_SUPPORTED;
};  // StatusCode

//  TODO Add member variable from server config.
//  VirtualServer is the entity processing request from client.
//  - Member variables
//      _statusCode: Store status code of server.
//
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

    void processRequest(Connection& clientConnection);

private:
    port_t _portNumber;
    std::string _name;
    std::size_t _clientMaxBodySize;
    std::vector<Location*> _location;

    std::map<std::string, std::string> _others;

    Connection* _connection;

    char _statusCode[4];
    std::string _targetRepresentationURI;

    void setStatusCode(const char* statusCode) { std::memcpy(this->_statusCode, statusCode, 4); };

    bool isStatusCode(const char* statusCode);
    bool isStatusEmpty();

    void processGETRequest(Connection& clientConnection);
    void processPOSTRequest(Connection& clientConnection);
    void processDELETERequest(Connection& clientConnection);

    void processLocation(const Request& request, const Location& location);

    void setResponseMessageByStatusCode(Connection& clientConnection);

    int setOKGETResponse(Connection& clientConnection);
};  // VirtualServer

inline bool VirtualServer::isStatusCode(const char* statusCode) {
    return memcmp(this->_statusCode, statusCode, 4) == 0;
}

inline bool VirtualServer::isStatusEmpty() {
    return memcmp(this->_statusCode, StatusCode::EMPTY, 4) == 0;
}

#endif  // VIRTUALSERVER_HPP_
