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

static void updateContentType(const std::string& name, std::string& type);
static void updateExtension(const std::string& name, std::string& extension);

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

//  get matching location for request.
//  - Parameters request: request to search.
//  - Return: matching location, if no location match, NULL would be returned.
const Location* VirtualServer::getMatchingLocation(const Request& request) {
    const std::string& targetResourceURI = request.getTargetResourceURI();

    for (std::vector<Location*>::const_iterator iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location* const & locationPointer = *iter;

        if (locationPointer->isRouteMatch(targetResourceURI))
            return locationPointer;
    }

    return NULL;
}

//  Process GET request.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processGET(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    struct stat buf;
    std::string targetRepresentationURI;

    const Location* locationPointer = this->getMatchingLocation(request);
    if (locationPointer == NULL)
        return this->set404Response(clientConnection);

    const Location& location = *locationPointer;

    if (!location.isRequestMethodAllowed(request.getMethod()))
        return this->set405Response(clientConnection, &location);

    location.updateRepresentationPath(targetResourceURI, targetRepresentationURI);
    if (stat(targetRepresentationURI.c_str(), &buf) == 0
            && (buf.st_mode & S_IFREG) != 0) {
        this->setStatusLine(clientConnection, Status::I_200);

        clientConnection.appendResponseMessage("Server: crash-webserve\r\n");
        clientConnection.appendResponseMessage("Date: ");
        clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Content-Type: ");
        std::string type;
        updateContentType(targetRepresentationURI, type);
        clientConnection.appendResponseMessage(type);
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Content-Length: ");
        std::ostringstream oss;
        oss << buf.st_size;
        clientConnection.appendResponseMessage(oss.str().c_str());
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Last-Modified: ");
        const struct timespec lastModified = buf.st_mtimespec;
        const struct tm tm = *gmtime(&lastModified.tv_sec);
        char lastModifiedString[BUF_SIZE];
        strftime(lastModifiedString, BUF_SIZE, "%a, %d %b %Y %H:%M:%S GMT", &tm);
        clientConnection.appendResponseMessage(lastModifiedString);
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
        clientConnection.appendResponseMessage("\r\n");

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
            && (buf.st_mode & S_IFREG) != 0) {
        this->setStatusLine(clientConnection, Status::I_200);

        clientConnection.appendResponseMessage("Server: crash-webserve\r\n");
        clientConnection.appendResponseMessage("Date: ");
        clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Content-Type: ");
        std::string type;
        updateContentType(targetRepresentationURI, type);
        clientConnection.appendResponseMessage(type);
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Content-Length: ");
        std::ostringstream oss;
        oss << buf.st_size;
        clientConnection.appendResponseMessage(oss.str().c_str());
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Last-Modified: ");
        const struct timespec lastModified = buf.st_mtimespec;
        const struct tm tm = *gmtime(&lastModified.tv_sec);
        char lastModifiedString[BUF_SIZE];
        strftime(lastModifiedString, BUF_SIZE, "%a, %d %b %Y %H:%M:%S GMT", &tm);
        clientConnection.appendResponseMessage(lastModifiedString);
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
        clientConnection.appendResponseMessage("\r\n");

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

    if (location.getAutoIndex()
            && stat(targetRepresentationURI.c_str(), &buf) == 0
            && (buf.st_mode & S_IFDIR) != 0)
        return this->setListResponse(clientConnection, targetRepresentationURI.c_str());

    return this->set404Response(clientConnection);
}

//  Process POST request.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processPOST(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    std::string targetRepresentationURI;

    const Location* locationPointer = this->getMatchingLocation(request);
    if (locationPointer == NULL)
        return this->set404Response(clientConnection);

    const Location& location = *locationPointer;

    location.updateRepresentationPath(targetResourceURI, targetRepresentationURI);
    if (!location.isRequestMethodAllowed(request.getMethod()))
        return this->set405Response(clientConnection, &location);

    std::ofstream out(targetRepresentationURI.c_str());
    if (!out.is_open())
        return -1;

    const std::string& requestBody = clientConnection.getRequest().getBody();
    out << requestBody;
    out.close();

    this->setStatusLine(clientConnection, Status::I_200);

    // TODO 적절한 헤더 필드 추가하기(content-length)
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    // TODO 적절한 바디 생성하기

    return 0;
}

//  Process DELETE request.
//  - Parameters request: The request to process.
//  - Return(None)
int VirtualServer::processDELETE(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    struct stat buf;
    std::string targetRepresentationURI;

    const Location* locationPointer = this->getMatchingLocation(request);
    if (locationPointer == NULL)
        return this->set404Response(clientConnection);

    const Location& location = *locationPointer;

    if (!location.isRequestMethodAllowed(request.getMethod()))
        return this->set405Response(clientConnection, &location);

    location.updateRepresentationPath(targetResourceURI, targetRepresentationURI);
    if (stat(targetRepresentationURI.c_str(), &buf) == 0) {

        if (unlink(targetRepresentationURI.c_str()) == -1)
            return -1;

        this->setStatusLine(clientConnection, Status::I_200);

        // TODO 적절한 헤더 필드 추가하기(content-length)
        clientConnection.appendResponseMessage("Date: ");
        clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
        clientConnection.appendResponseMessage("\r\n\r\n");

        // TODO 적절한 바디 설정하기

        return 0;
    }

    return this->set404Response(clientConnection);
}

//  Set status line to response of clientConnection.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::setStatusLine(Connection& clientConnection, Status::Index index) {
    clientConnection.appendResponseMessage("HTTP/1.1 ");
    clientConnection.appendResponseMessage(HTTP::getStatusCodeBy(index));
    clientConnection.appendResponseMessage(" ");
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
    clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    return 0;
}

//  set response message with 405 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
int VirtualServer::set405Response(Connection& clientConnection, const Location* location) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_405);

    // TODO implement
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n");

    clientConnection.appendResponseMessage("Allow: ");
    std::string tAllowMethod = "";
    if (location->isRequestMethodAllowed(HTTP::RM_GET))
        tAllowMethod += "GET, ";
    if (location->isRequestMethodAllowed(HTTP::RM_POST))
        tAllowMethod += "POST, ";
    if (location->isRequestMethodAllowed(HTTP::RM_DELETE))
        tAllowMethod += "DELETE, ";
    tAllowMethod = tAllowMethod.substr(0, tAllowMethod.find_last_of(","));
    clientConnection.appendResponseMessage(tAllowMethod);
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
    clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n\r\n");

    return 0;
}

int VirtualServer::setListResponse(Connection& clientConnection, const std::string& path) {
    clientConnection.clearResponseMessage();
    this->setStatusLine(clientConnection, Status::I_200);

    DIR* dir;

    dir = opendir(path.c_str());
    if (dir == NULL)
        return -1;

    int contentLength = 100;
    while (true) {
        const struct dirent* entry = readdir(dir);
        if (entry == NULL)
            break;
        if (entry->d_namlen == 1 && strcmp(entry->d_name, ".") == 0)
            continue;

        const bool isEntryDirectory = (entry->d_type == DT_DIR);
        contentLength += (entry->d_namlen + isEntryDirectory) * 2 + 17;
    }
    closedir(dir);

    contentLength += 24;

    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << contentLength;
    clientConnection.appendResponseMessage(oss.str().c_str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("Content-Type: text/html\r\n");
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(this->makeHeaderField(HTTP::DATE));
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Server: crash-webserve\r\n");
    clientConnection.appendResponseMessage("\r\n");

    clientConnection.appendResponseMessage("<html>\r\n<head><title>Index of /</title></head>\r\n<body bgcolor=\"white\">\r\n<h1>Index of /</h1><hr><pre>");

    dir = opendir(path.c_str());
    if (dir == NULL)
        return -1;

    while (true) {
        const struct dirent* entry = readdir(dir);
        if (entry == NULL)
            break;
        if (entry->d_namlen == 1 && strcmp(entry->d_name, ".") == 0)
            continue;

        const bool isEntryDirectory = (entry->d_type == DT_DIR);
        std::string name;
        if (!isEntryDirectory)
            name = entry->d_name;
        else {
            name += entry->d_name;
            name += "/";
        }

        clientConnection.appendResponseMessage("<a href=\"");
        clientConnection.appendResponseMessage(name);
        clientConnection.appendResponseMessage("\">");
        clientConnection.appendResponseMessage(name);
        clientConnection.appendResponseMessage("</a>\r\n");
    }
    closedir(dir);

    clientConnection.appendResponseMessage("</pre><hr></body></html>");

    return 0;
}

std::string VirtualServer::makeHeaderField(unsigned short fieldName) {
    switch (fieldName)
    {
    case HTTP::DATE:
        return makeDateHeaderField();
    // case HTTP::ALLOW:
    //     return makeAllowHeaderField();
    }
    return ""; // TODO delete
}

// Find the current time based on GMT
//  - Parameters(None)
//  - Return
//      Current time based on GMT(std::string)
std::string VirtualServer::makeDateHeaderField() {
    char cDate[1000];
    time_t rr = time(0);
    struct tm tm = *gmtime(&rr);
    strftime(cDate, sizeof(cDate), "%a, %d %b %Y %H:%M:%S GMT", &tm);
    std::string dateStr = cDate;
    return dateStr;
}

// std::string Connection::makeAllowHeaderField() {
    
// }

//  set 'type' of 'name'
//  - Parameters
//      name: name of file
//      type: type to set
//  - Return(None)
static void updateContentType(const std::string& name, std::string& type) {
    std::string extension;
    updateExtension(name, extension);
    if (std::strcmp(extension.c_str(), "txt") == 0)
        type = "text/plain";
    else if (std::strcmp(extension.c_str(), "html") == 0)
        type = "text/html";
    else
        type = "application/octet-stream";
}

//  set 'extension' of 'name'
//  - Parameters
//      name: name of file
//      type: type to set
//  - Return(None)
static void updateExtension(const std::string& name, std::string& extension) {
    const std::string::size_type extensionBeginPosition = name.rfind('.') + 1;
    extension.clear();
    if (extensionBeginPosition !=  std::string::npos)
        extension = name.c_str() + extensionBeginPosition;
    else
        return;
}
