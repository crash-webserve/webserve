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

namespace HTTP {

//  status code와 reason phrase를 저장하기 위한 기본 단위 구조체
//  - Member
//      enum Index: Status::_array 배열의 index로 넣어줄 용도의 상수
//      _array: status code와 reason phrase 문자열들의 주소를 struct Status 단위로 저장할 전역 변수
//
//      _statusCode: Status 객체의 status code
//      _reasonPhrase: Status 객체의 reason phrase
struct Status {
    enum Index {
        I_000,
        I_200,
        I_201,
        I_301,
        I_400,
        I_403,
        I_404,
        I_405,
        I_411,
        I_413,
        I_500,
    };

    static const Status _array[];

    const char* _statusCode;
    const char* _reasonPhrase;
};  // Status

inline const HTTP::Status& getStatusBy(HTTP::Status::Index index) {
    return HTTP::Status::_array[index];
}

inline const char* getStatusCodeBy(HTTP::Status::Index index) {
    return HTTP::getStatusBy(index)._statusCode;
}

inline const char* getStatusReasonBy(HTTP::Status::Index index) {
    return HTTP::getStatusBy(index)._reasonPhrase;
}

}  // HTTP

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
//      _statusCode: Store status code of server.
class VirtualServer {
public:
    enum ReturnCode {
        RC_SUCCESS,
        RC_IN_PROGRESS,
    };  // ReturnCode

    VirtualServer();
    VirtualServer(port_t portNumber, const std::string& name);

    port_t getPortNumber() const { return this->_portNumber; }
    std::string getServerName() const { return this->_name; }
    void setPortNumber(port_t portNumber) { this->_portNumber = portNumber; }
    void setServerName(std::string serverName) { this->_name = serverName; }
    void setClientMaxBodySize(std::size_t clientMaxBodySize) { this->_clientMaxBodySize = clientMaxBodySize; };
    void setOtherDirective(std::string directiveName, std::vector<std::string> directiveValue) { 
        _others.insert(make_pair(directiveName, directiveValue[0])); // TODO multi-value
    };
    void appendLocation(Location* lc) { this->_location.push_back(lc); };
    VirtualServer::ReturnCode processRequest(Connection& clientConnection);

    std::string makeHeaderField(unsigned short fieldName);
    std::string makeDateHeaderField();
    // std::string makeAllowHeaderField();

private:
    port_t _portNumber;
    std::string _name;
    std::size_t _clientMaxBodySize;
    std::vector<Location*> _location;

    std::map<std::string, std::string> _others;

    const Location* getMatchingLocation(const Request& request);

    int processGET(Connection& clientConnection);
    int processPOST(Connection& clientConnection);
    int processDELETE(Connection& clientConnection);

    void setStatusLine(Connection& clientConnection, HTTP::Status::Index index);

    int set404Response(Connection& clientConnection);
    int set405Response(Connection& clientConnection, const Location* locations);
    int set500Response(Connection& clientConnection);
    int setListResponse(Connection& clientConnection, const std::string& path);
};  // VirtualServer

#endif  // VIRTUALSERVER_HPP_
