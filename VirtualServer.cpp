#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <dirent.h>
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
VirtualServer::VirtualServer(port_t portNumber, const std::string& name)
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

//  Process GET request.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processGET(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    struct stat buf;
    std::string targetRepresentationURI;

    std::vector<Location*>::const_iterator iter;
    for (iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location& location = **iter;

        if (!location.isRouteMatch(targetResourceURI))
            continue;

        if (!location.isRequestMethodAllowed(request.getMethod()))
            return this->set405Response(clientConnection);

        location.getRepresentationPath(targetResourceURI, targetRepresentationURI);
        if (stat(targetRepresentationURI.c_str(), &buf) == 0
                && (buf.st_mode & S_IFDIR) != 0) {
            this->setStatusLine(clientConnection, Status::I_200);

            // TODO 적절한 헤더 필드 추가하기(content-length)
            clientConnection.appendResponseMessage("Date: ");
            clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
            clientConnection.appendResponseMessage("\r\n\r\n");

            std::ifstream targetRepresentation(targetRepresentationURI, std::ios_base::binary | std::ios_base::ate);
            if (!targetRepresentation.is_open())
                return -1;

            std::ifstream::pos_type size = targetRepresentation.tellg();
            std::string str(size, '\0');
            targetRepresentation.seekg(0);
            if (targetRepresentation.read(&str[0], size))
                clientConnection.appendResponseMessage(str.c_str());

            targetRepresentation.close();

            return 0;
        }

        if (stat(location.getIndex().c_str(), &buf) == 0
                && (buf.st_mode & S_IFDIR) != 0) {
            this->setStatusLine(clientConnection, Status::I_200);

            // TODO 적절한 헤더 필드 추가하기(content-length)
            clientConnection.appendResponseMessage("Date: ");
            clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
            clientConnection.appendResponseMessage("\r\n\r\n");

            std::ifstream targetRepresentation(location.getIndex(), std::ios_base::binary | std::ios_base::ate);
            if (!targetRepresentation.is_open())
                return -1;

            std::ifstream::pos_type size = targetRepresentation.tellg();
            std::string str(size, '\0');
            targetRepresentation.seekg(0);
            if (targetRepresentation.read(&str[0], size))
                clientConnection.appendResponseMessage(str.c_str());

            targetRepresentation.close();

            return 0;
        }

        if (location.getAutoIndex())
            return this->setListResponse(clientConnection, location.getRoot());
    }

    return this->set404Response(clientConnection);
}

//  Process POST request.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processPOST(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    std::string targetRepresentationURI;

    std::vector<Location*>::const_iterator iter;
    for (iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location& location = **iter;

        if (!location.isRouteMatch(targetResourceURI))
            continue;

        location.getRepresentationPath(targetResourceURI, targetRepresentationURI);
        if (!location.isRequestMethodAllowed(request.getMethod()))
            return this->set405Response(clientConnection);

        std::ofstream out(targetRepresentationURI.c_str());
        if (!out.is_open())
            return -1;

        const std::string& requestBody = clientConnection.getRequest().getBody();
        out << requestBody;
        out.close();

        this->setStatusLine(clientConnection, Status::I_200);

        // TODO 적절한 헤더 필드 추가하기(content-length)
        clientConnection.appendResponseMessage("Date: ");
        clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
        clientConnection.appendResponseMessage("\r\n\r\n");

        // TODO 적절한 바디 생성하기

        return 0;
    }

    return this->set405Response(clientConnection);
}

//  Process DELETE request.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processDELETE(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    struct stat buf;
    std::string targetRepresentationURI;

    std::vector<Location*>::const_iterator iter;
    for (iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location& location = **iter;

        if (!location.isRouteMatch(targetResourceURI))
            continue;

        location.getRepresentationPath(targetResourceURI, targetRepresentationURI);
        if (stat(targetRepresentationURI.c_str(), &buf) == 0
                && (buf.st_mode & S_IFDIR) != 0) {
            if (!location.isRequestMethodAllowed(request.getMethod()))
                return this->set405Response(clientConnection);

            if (unlink(targetRepresentationURI.c_str()) == -1)
                return this->set500Response(clientConnection);

            this->setStatusLine(clientConnection, Status::I_200);

            // TODO 적절한 헤더 필드 추가하기(content-length)
            clientConnection.appendResponseMessage("Date: ");
            clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
            clientConnection.appendResponseMessage("\r\n\r\n");

            // TODO 적절한 바디 설정하기

            return 0;
        }
    }

    return this->set404Response(clientConnection);
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
int VirtualServer::set404Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_404);

    // TODO implement
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    return 0;
}

//  set response message with 405 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
int VirtualServer::set405Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_405);

    // TODO implement
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    return 0;
}

//  set response message with 500 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
int VirtualServer::set500Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_500);

    // TODO append header section and body
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    return 0;
}

int VirtualServer::setListResponse(Connection& clientConnection, const std::string& path) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_200);

    // TODO 필요에 따라 헤더 추가하기
    clientConnection.appendResponseMessage("Transfer-Encoding: chunked\r\n");
    clientConnection.appendResponseMessage("Content-Type: text/plain");
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    DIR* dir = opendir(path.c_str());
    if (dir == NULL)
        return -1;

    while (true) {
        const struct dirent* entry = readdir(dir);
        if (entry == NULL)
            break;

        std::stringstream iss;
        iss << entry->d_namlen + 1;
        clientConnection.appendResponseMessage(iss.str().c_str());
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage(entry->d_name);
        clientConnection.appendResponseMessage("\n\r\n");
    }
    clientConnection.appendResponseMessage("0\r\n\r\n");

    closedir(dir);

    return 0;
}
