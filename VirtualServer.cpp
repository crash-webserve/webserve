#include <fstream>
#include <cstdio>
#include <cstring>
#include "VirtualServer.hpp"
#include "Connection.hpp"
#include "constant.hpp"

using HTTP::Status;

const Status Status::_array[] = {
    { "000", "default" },
    { "200", "ok" },
    { "201", "created" },
    { "301", "moved permanently" },
    { "400", "bad request" },
    { "403", "forbidden" },
    { "404", "not found" },
    { "405", "method not allowed" },
    { "411", "length required" },
    { "413", "payload too large" },
    { "500", "internal server error" },
    { "505", "http version not supported" },
};

//  Default constructor of VirtualServer.
//  - Parameters(None)
VirtualServer::VirtualServer() 
: _portNumber(0),
_name("")
{
} 

//  Constructor of VirtualServer.
//  - Parameters
//      portNumber: The port number.
//      serverName: The server name.
VirtualServer::VirtualServer(short portNumber, const std::string& name)
: _portNumber(portNumber), _name(name) {
}

//  Process request from client.
//  - Parameters
//      clientConnection: The connection of client requesting process.
//      kqueueFD: The kqueue fd is where to add write event for response.
//  - Return: See the type definition.
VirtualServer::ReturnCode VirtualServer::processRequest(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();

    for (std::vector<Location*>::const_iterator iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location& location = **iter;
        if (location.isPathMatch(targetResourceURI)) {
            this->processLocation(request, location);
            break;
        }
    }

    if (this->isStatusDefault())
        this->setStatus(Status::I_404);

    this->processPOSTRequest(request);
    this->processDELETERequest(request);

    this->setResponseMessageByStatus(clientConnection);

    if (this->isStatus(Status::I_500))
        this->set500Response(clientConnection);

    return VirtualServer::RC_SUCCESS;
}

//  Process request about location.
//  - Parameters
//      request: The request to process.
//      location: The location matched to the target resource URI.
//  - Return(None)
void VirtualServer::processLocation(const Request& request, const Location& location) {
    if (!location.isRequestMethodAllowed(request.getMethod())) {
        this->setStatus(Status::I_405);
        return;
    }

    this->setStatus(Status::I_404); // TODO 강제로 404 만드는 임시 코드

    const std::string& targetResourceURI = request.getTargetResourceURI();
    location.getRepresentationPath(targetResourceURI, this->_targetRepresentationURI);
    // TODO 만약 _targetRepresentationURI가 존재하는 파일이라면 상태를 200으로 설정
    // TODO (추측) 만약 _targetRepresentationURI가 존재하지 않는 파일일 경우 index에 정의된 파일로 설정하기. 그것도 없으면 404.(index 관련 로직 조사 필요 POST라면? DELETE라면?)
}

//  Process POST request's unique work.
//  If the request method is not POST, do nothing.
//  - Parameters request: The request to process.
//  - Return(None)
void VirtualServer::processPOSTRequest(const Request& request) {
    if (request.getMethod() != HTTP::RM_POST)
        return;

    std::ofstream out(this->_targetRepresentationURI.c_str());
    if (!out.is_open()) {
        this->setStatus(Status::I_500);
    }

    const std::string& requestBody = request.getBody();
    out << requestBody;
    out.close();
}

//  Process DELETE request's unique work.
//  If the request method is not DELETE, do nothing.
//  - Parameters request: The request to process.
//  - Return(None)
void VirtualServer::processDELETERequest(const Request& request) {
    if (request.getMethod() != HTTP::RM_DELETE)
        return;

    unlink(this->_targetRepresentationURI.c_str());
}

//  Set response message according to this->_statusCode.
//  If the status code is 500, do nothing.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
void VirtualServer::setResponseMessageByStatus(Connection& clientConnection) {
    if (this->isStatus(Status::I_500))
        return;

    clientConnection.clearResponseMessage();

    setStatusLine(clientConnection);

    if (this->isStatus(Status::I_200)) {
        if (clientConnection.getRequest().getMethod() == HTTP::RM_GET) {
            this->setGETResponse(clientConnection);
        }
        else if (clientConnection.getRequest().getMethod() == HTTP::RM_POST) {
            this->setPOSTResponse(clientConnection);
        }
        else if (clientConnection.getRequest().getMethod() == HTTP::RM_DELETE) {
            this->setDELETEResponse(clientConnection);
        }
    }
    else if (this->isStatus(Status::I_404)) {
        this->set404Response(clientConnection);
    }
}

//  Set status line to response of clientConnection.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::setStatusLine(Connection& clientConnection) {
    clientConnection.appendResponseMessage("HTTP/1.1 ");
    clientConnection.appendResponseMessage(HTTP::getStatusCodeBy(Status::I_200));
    clientConnection.appendResponseMessage(HTTP::getStatusReasonBy(Status::I_200));
    clientConnection.appendResponseMessage("\r\n");
}

//  Set response of GET request.
//  - Parameters clientConnection: The client connection.
//  - Return: whether an error has occured or not.
int VirtualServer::setGETResponse(Connection& clientConnection) {
    // TODO 적절한 헤더 필드 추가하기
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));

    std::ifstream targetRepresentation(this->_targetRepresentationURI, std::ios_base::binary | std::ios_base::ate);
    if (!targetRepresentation.is_open()) {
        this->setStatus(Status::I_500);
        return -1;
    }

    std::ifstream::pos_type size = targetRepresentation.tellg();
    std::string str(size, '\0');
    targetRepresentation.seekg(0);
    if (targetRepresentation.read(&str[0], size))
        clientConnection.appendResponseMessage(str.c_str());

    return 0;
}

//  Set response of POST request.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::setPOSTResponse(Connection& clientConnection) {
    // TODO implement
}

//  Set response of DELETE request.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::setDELETEResponse(Connection& clientConnection) {
    // TODO implement
}

void VirtualServer::set404Response(Connection& clientConnection) {
    // TODO implement
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");
}

//  set response message with 500 status.
//  If the status code is not 500, do nothing.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
void VirtualServer::set500Response(Connection& clientConnection) {
    if (!this->isStatus(Status::I_500))
        return;

    clientConnection.clearResponseMessage();

    setStatusLine(clientConnection);

    // TODO append header section and body
}
