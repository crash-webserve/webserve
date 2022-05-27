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
    int returnCode = 0;

    switch(clientConnection.getRequest().getMethod()) {
        case HTTP::RM_GET:
            returnCode = processGET(clientConnection);
            break;
        case HTTP::RM_POST:
            returnCode = processPOST(clientConnection);
            break;
        case HTTP::RM_DELETE:
            returnCode = processDELETE(clientConnection);
            break;
        default:
            break;
    }

    if (returnCode == -1)
        this->set500Response(clientConnection);

    return VirtualServer::RC_SUCCESS;
}

int VirtualServer::processGET(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();

    for (std::vector<Location*>::const_iterator iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location& location = **iter;

        if (location.isRouteMatch(targetResourceURI)) {
            if (!location.isRequestMethodAllowed(request.getMethod())) {
                this->set405Response(clientConnection);
                return 0;
            }

            this->set404Response(clientConnection); // TODO 강제로 404 만드는 임시 코드
            return 0;   // TODO 임시 코드

            const std::string& targetResourceURI = request.getTargetResourceURI();
            location.getRepresentationPath(targetResourceURI, this->_targetRepresentationURI);
            // TODO 만약 _targetRepresentationURI가 존재하는 파일이라면 상태를 200으로 설정
            // TODO (추측) 만약 _targetRepresentationURI가 존재하지 않는 파일일 경우 index에 정의된 파일로 설정하기. 그것도 없으면 404.(index 관련 로직 조사 필요 POST라면? DELETE라면?)
            break;
        }
    }

    this->setStatusLine(clientConnection, Status::I_200);

    // TODO 적절한 헤더 필드 추가하기(content-length)
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    std::ifstream targetRepresentation(this->_targetRepresentationURI, std::ios_base::binary | std::ios_base::ate);
    if (!targetRepresentation.is_open())
        return -1;

    std::ifstream::pos_type size = targetRepresentation.tellg();
    std::string str(size, '\0');
    targetRepresentation.seekg(0);
    if (targetRepresentation.read(&str[0], size))
        clientConnection.appendResponseMessage(str.c_str());

    return 0;
}

//  Process POST request's unique work.
//  If the request method is not POST, do nothing.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processPOST(Connection& clientConnection) {
    // TODO implement
    std::ofstream out(this->_targetRepresentationURI.c_str());
    if (!out.is_open())
        return -1;

    const std::string& requestBody = clientConnection.getRequest().getBody();
    out << requestBody;
    out.close();

    return 0;
}

//  Process DELETE request's unique work.
//  If the request method is not DELETE, do nothing.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processDELETE(Connection& clientConnection) {
    // TODO implement
    unlink(this->_targetRepresentationURI.c_str());

    return 0;
}

//  Set status line to response of clientConnection.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::setStatusLine(Connection& clientConnection, Status::Index index) {
    clientConnection.appendResponseMessage("HTTP/1.1 ");
    clientConnection.appendResponseMessage(HTTP::getStatusCodeBy(index));
    clientConnection.appendResponseMessage(HTTP::getStatusReasonBy(index));
    clientConnection.appendResponseMessage("\r\n");
}

//  set response message with 404 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::set404Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_404);

    // TODO implement
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");
}

//  set response message with 405 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::set405Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_405);

    // TODO implement
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");
}

//  set response message with 500 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::set500Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_500);
    //
    // TODO append header section and body
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");
}
