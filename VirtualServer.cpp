#include <fstream>
#include <cstring>
#include "VirtualServer.hpp"
#include "Connection.hpp"
#include "constant.hpp"

const char* StatusCode::EMPTY = "000";
const char* StatusCode::OK = "200";
const char* StatusCode::CREATED = "201";
const char* StatusCode::MOVED_PERMANENTLY = "301";
const char* StatusCode::BAD_REQUEST = "400";
const char* StatusCode::FORBIDDEN = "403";
const char* StatusCode::NOT_FOUND = "404";
const char* StatusCode::METHOD_NOT_ALLOWED = "405";
const char* StatusCode::LENGTH_REQUIRED = "411";
const char* StatusCode::PAYLOAD_TOO_LARGE = "413";
const char* StatusCode::INTERNAL_SERVER_ERROR = "500";
const char* StatusCode::HTTP_VERSION_NOT_SUPPORTED = "505";

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
void VirtualServer::processRequest(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();

    this->setStatusCode(StatusCode::EMPTY);
    switch (request.getMethod()) {
        case HTTP::RM_GET:
            this->processGETRequest(clientConnection);
            break;
        case HTTP::RM_POST:
            this->processPOSTRequest(clientConnection);
            break;
        case HTTP::RM_DELETE:
            this->processDELETERequest(clientConnection);
            break;
        case HTTP::RM_UNKNOWN:
            // TODO 평가지에 unknown이면 뭐라고 나와 있었는지 참고하기
            break;
        default:
            assert(false);
            break;
    }

    this->setResponseMessageByStatusCode(clientConnection);
}

//  Process GET request.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
void VirtualServer::processGETRequest(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();

    for (std::vector<Location*>::const_iterator iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location& location = **iter;
        if (location.isPathMatch(targetResourceURI)) {
            this->processLocation(request, location);
            break;
        }
    }

    if (this->isStatusEmpty()) {
        this->setStatusCode(StatusCode::NOT_FOUND);
        return;
    }
}

//  Process POST request.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
void VirtualServer::processPOSTRequest(Connection& clientConnection) {
    // TODO Implement
    (void)clientConnection;
}

//  Process DELETE request.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
void VirtualServer::processDELETERequest(Connection& clientConnection) {
    // TODO Implement
    (void)clientConnection;
}

//  Process request about location.
//  - Parameters
//      request: The request to process.
//      location: The location matched to the target resource URI.
//  - Return(None)
void VirtualServer::processLocation(const Request& request, const Location& location) {
    if (!location.isRequestMethodAllowed(request.getMethod())) {
        this->setStatusCode(StatusCode::METHOD_NOT_ALLOWED);
        return;
    }

    const std::string& targetResourceURI = request.getTargetResourceURI();
    location.getRepresentationPath(targetResourceURI, this->_targetRepresentationURI);
}

//  Set response message according to this->_statusCode.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
void VirtualServer::setResponseMessageByStatusCode(Connection& clientConnection) {
    if (this->isStatusCode(StatusCode::OK))
        setOKGETResponse(clientConnection);
}

//  Set response message as OK status response of GET request.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
int VirtualServer::setOKGETResponse(Connection& clientConnection) {
    clientConnection.appendResponseMessage("HTTP/1.1 ");
    clientConnection.appendResponseMessage(StatusCode::OK);
    clientConnection.appendResponseMessage(" OK\r\n");

    // TODO append header section

    std::ifstream targetRepresentation(this->_targetRepresentationURI, std::ios_base::binary | std::ios_base::ate);
    if (!targetRepresentation.is_open()) {
        setStatusCode(StatusCode::INTERNAL_SERVER_ERROR);
        return -1;
    }

    std::ifstream::pos_type size = targetRepresentation.tellg();
    std::string str(size, '\0');
    targetRepresentation.seekg(0);
    if (targetRepresentation.read(&str[0], size))
        clientConnection.appendResponseMessage(str.c_str());

    return 0;
}
