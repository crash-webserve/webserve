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

//  status code와 reason phrase를 저장하기 위한 기본 단위 구조체
//  - Member
//      enum Index: Status::_array 배열의 index로 넣어줄 용도의 상수
//      _array: status code와 reason phrase 문자열들의 주소를 struct Status 단위로 저장할 전역 변수
//
//      _statusCode: Status 객체의 status code
//      _reasonPhrase: Status 객체의 reason phrase
struct Status {
    enum Index {
        SI_DEFAULT,
        SI_OK,
        SI_CREATED,
        SI_MOVED_PERMANENTLY,
        SI_BAD_REQUEST,
        SI_FORBIDDEN,
        SI_NOT_FOUND,
        SI_METHOD_NOT_ALLOWED,
        SI_LENGTH_REQUIRED,
        SI_PAYLOAD_TOO_LARGE,
        SI_INTERNAL_SERVER_ERROR,
    };

    static const Status _array[];

    const char* _statusCode;
    const char* _reasonPhrase;
};  // Status

//  TODO Add member variable from server config.
//  VirtualServer is the entity processing request from client.
//  - Member variables
//      enum ReturnCode: The return code for this->processRequest().
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
//
//      _statusCode: Store status code of server.
//      _targetRepresentationURI: Store target representation URI.
class VirtualServer {
public:
    enum ReturnCode {
        RC_SUCCESS,
        RC_IN_PROGRESS,
    };  // ReturnCode

    VirtualServer();
    VirtualServer(short portNumber, const std::string& name);

    port_t getPortNumber() const { return this->_portNumber; }
    std::string getServerName() const { return this->_name; }
    void setPortNumber(port_t portNumber) { this->_portNumber = portNumber; }
    void setServerName(std::string serverName) { this->_name = serverName; }
    void setConnection(Connection* connection) { this->_connection = connection; };
    void setClientMaxBodySize(std::size_t clientMaxBodySize) { this->_clientMaxBodySize = clientMaxBodySize; };
    void setOtherDirective(std::string directiveName, std::vector<std::string> directiveValue) { 
        _others.insert(make_pair(directiveName, directiveValue[0])); // TODO multi-value
    };
    void appendLocation(Location* lc) { this->_location.push_back(lc); };
    VirtualServer::ReturnCode processRequest(Connection& clientConnection);

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
    bool isStatusDefault();

    void processGETRequest(Connection& clientConnection);
    void processPOSTRequest(Connection& clientConnection);
    void processDELETERequest(Connection& clientConnection);

    void processLocation(const Request& request, const Location& location);

    void setResponseMessageByStatusCode(Connection& clientConnection);

    int setOKGETResponse(Connection& clientConnection);
};  // VirtualServer

//  Return whether the VirtualServer object's _statusCode is same with statusCode.
//  - Parameters statusCode: the status code to compare with virtual server one.
//  - Return: Whether the VirtualServer object's _statusCode is same with statusCode.
inline bool VirtualServer::isStatusCode(const char* statusCode) {
    return memcmp(this->_statusCode, statusCode, 4) == 0;
}

//  Return whether the VirtualServer object's _statusCode is same with default statusCode.
//  - Parameters statusCode(None)
//  - Return: Whether the VirtualServer object's _statusCode is same with default statusCode.
inline bool VirtualServer::isStatusDefault() {
    return memcmp(this->_statusCode, Status::_array[Status::SI_DEFAULT]._statusCode, 4) == 0;
}

#endif  // VIRTUALSERVER_HPP_
