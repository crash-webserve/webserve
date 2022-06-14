#include <fcntl.h>
#include "VirtualServer.hpp"
#include "EventHandler.hpp"
#include "EventContext.hpp"
#include "constant.hpp"

static void insertToStringMap(StringMap& envMap, std::string key, std::string value);

using HTTP::Status;

const Status Status::_array[] = {
    { "000", "default" },
    { "200", "ok" },
    { "201", "created" },
    { "301", "moved permanently" },
    { "308", "Permanent Redirect" },
    { "400", "bad request" },
    { "404", "not found" },
    { "405", "method not allowed" },
    { "411", "length required" },
    { "413", "payload too large" },
    { "500", "internal server error" },
};

static void updateContentType(const std::string& name, std::string& type);
static void updateExtension(const std::string& name, std::string& extension);
static void updateBodyString(HTTP::Status::Index index, const char* description, std::string& bodyString);
//  Default constructor of VirtualServer.
//  - Parameters(None)
VirtualServer::VirtualServer()
: _portNumber(0),
_name(""),
_clientMaxBodySize(DEFAULT_CLIENT_MAX_BODY_SIZE)
{
} 

//  Constructor of VirtualServer.
//  - Parameters
//      portNumber: The port number.
//      serverName: The server name.
VirtualServer::VirtualServer(port_t portNumber, const std::string& name)
: _portNumber(portNumber), _name(name) {
}

//  update error page of virtual server.
//  - Parameters
//      statusCode: status code in std::string.
//      filePath: the path of file to read.
//  - Return: upon successful completion a value of 0 is returned.
//      otherwise, a value of -1 is returned.
int VirtualServer::updateErrorPage(EventHandler& eventHandler, const std::string& statusCode, const std::string& filePath) {
    const int targetFileFD = open(filePath.c_str(), O_RDONLY);
    if (targetFileFD == -1)
        return -1;
    if (fcntl(targetFileFD, F_SETFL, O_NONBLOCK) == -1) {
        close(targetFileFD);
        return -1;
    }

    std::pair<const std::string&, VirtualServer&>* tempData = new std::pair<const std::string&, VirtualServer&>(statusCode, *this);
    eventHandler.addEvent(EVFILT_READ, targetFileFD, EventContext::EV_SetVirtualServerErrorPage, tempData);

    return 0;
}

//  event function reading a file and setting error page.
//  - Parameters context: context of event.
//  - Return: result of event.
EventContext::EventResult VirtualServer::eventSetVirtualServerErrorPage(EventContext& context) {
    char buf[BUF_SIZE];
    ssize_t readByteCount;
    const int targetFileFD = context.getIdent();
    std::pair<const std::string&, VirtualServer&>* data = static_cast<std::pair<const std::string&, VirtualServer&>*>(context.getData());
    const std::string& statusCode = data->first;

    readByteCount = read(targetFileFD, buf, BUF_SIZE - 1);
    if (readByteCount == -1) {
        delete data;
        delete &context;
        close(targetFileFD);
        return EventContext::ER_Done;
    }
    buf[readByteCount] = '\0';

    this->_errorPage[statusCode].append(buf);

    if (readByteCount == BUF_SIZE - 1)
        return EventContext::ER_Continue;
    else {
        delete data;
        delete &context;
        close(targetFileFD);
        return EventContext::ER_Done;
    }
}

//  Process request from client.
//  - Parameters
//      clientConnection: The connection of client requesting process.
//      kqueueFD: The kqueue fd is where to add write event for response.
//  - Return: See the type definition.
VirtualServer::ReturnCode VirtualServer::processRequest(Connection& clientConnection, EventHandler& eventHandler) {
    const Request& request = clientConnection.getRequest();
    ReturnCode returnCode;

    if (request.isParsingFail()) {
        returnCode = this->set400Response(clientConnection);
        if (returnCode == RC_ERROR)
            returnCode = this->set500Response(clientConnection);
        return returnCode;
    }
    else if (request.isLengthRequired()) {
        returnCode = this->set411Response(clientConnection);
        if (returnCode == RC_ERROR)
            returnCode = this->set500Response(clientConnection);
        return returnCode;
    }

    switch(request.getMethod()) {
        case HTTP::RM_GET:
            returnCode = processGET(clientConnection, eventHandler);
            break;
        case HTTP::RM_POST:
            returnCode = processPOST(clientConnection, eventHandler);
            break;
        case HTTP::RM_DELETE:
            returnCode = processDELETE(clientConnection);
            break;
        case HTTP::RM_PUT:
            returnCode = set201Response(clientConnection);
            break;
        default:
            returnCode = set405Response(clientConnection, NULL);
            break;
    }

    if (returnCode == RC_ERROR)
        returnCode = this->set500Response(clientConnection);

    return returnCode;
}

//  get matching location for request.
//  - Parameters request: request to search.
//  - Return: matching location, if no location match, NULL would be returned.
const Location* VirtualServer::getMatchingLocation(const Request& request) {
    const std::string& targetResourceURI = request.getTargetResourceURI();
    std::map<std::string, const Location*, std::greater<std::string> > matchingLongestRoute;
    for (std::vector<Location*>::const_iterator iter = this->_location.begin(); iter != this->_location.end(); ++iter) {
        const Location* const & locationPointer = *iter;

        if (locationPointer->isRouteMatch(targetResourceURI))
            matchingLongestRoute.insert(make_pair(locationPointer->getRoute(), locationPointer));
    }
    if (matchingLongestRoute.size())
        return matchingLongestRoute.begin()->second;
    return NULL;
}

bool VirtualServer::detectCGI(Connection& clientConnection, const Location& location, const std::string& targetResourceURI) {
    std::string targetExtention;
    std::vector<std::string> cgiExtensionOnLocation = location.getCGIExtention();
    std::vector<std::string>::iterator findResult;
    updateExtension(targetResourceURI, targetExtention);
    findResult = std::find(
        cgiExtensionOnLocation.begin(),
        cgiExtensionOnLocation.end(),
        targetExtention
    );
    if ( findResult == cgiExtensionOnLocation.end()) {
        return false;
    } else {
        clientConnection.parseCGIurl(targetResourceURI, targetExtention);
        this->fillCGIEnvMap(clientConnection, location);
        return true;
    }
}

//  Process GET request.
//  - Parameters request: The request to process.
//  - Return(None)
VirtualServer::ReturnCode VirtualServer::processGET(Connection& clientConnection, EventHandler& eventHandler) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    struct stat buf;
    std::string targetRepresentationURI;

    if (this->_others.find("return") != this->_others.end()) {
        if (this->_others.find("return")->second.front().compare("308") == 0)
            return this->set308Response(clientConnection, this->_others);
        return this->set301Response(clientConnection,  this->_others);
    }
    const Location* locationPointer = this->getMatchingLocation(request);
    if (locationPointer == NULL)
        return this->set404Response(clientConnection);
    const Location& location = *locationPointer;
    const std::map<std::string, std::vector<std::string> > &locOthers = location.getOtherDirective();
    if (locOthers.find("return") != locOthers.end()) {
        if (locOthers.find("return")->second.front().compare("308") == 0)
            return this->set308Response(clientConnection, locOthers);
        return this->set301Response(clientConnection, locOthers);
    }
    if (!location.isRequestMethodAllowed(request.getMethod()))
        return this->set405Response(clientConnection, &location);
    int targetClientMaxBodysize = location.getClientMaxBodySize();
    if (targetClientMaxBodysize < 0)
        targetClientMaxBodysize = this->_clientMaxBodySize;
    if (request.getBody().length() > static_cast<std::string::size_type>(targetClientMaxBodysize))
        return this->set413Response(clientConnection);

    location.updateRepresentationPath(targetResourceURI, targetRepresentationURI);

    if (this->detectCGI(clientConnection, location, targetResourceURI) == true) {
        return this->passCGI(clientConnection, location);
    }

    if (stat(targetRepresentationURI.c_str(), &buf) == 0
            && (buf.st_mode & S_IFREG) != 0) {
        this->appendStatusLine(clientConnection, Status::I_200);

        this->appendDefaultHeaderFields(clientConnection);
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

        if (buf.st_size == 0)
            return RC_SUCCESS;

        const int targetFileFD = open(targetRepresentationURI.c_str(), O_RDONLY);
        if (targetFileFD == -1)
            return RC_ERROR;
        if (fcntl(targetFileFD, F_SETFL, O_NONBLOCK) == -1) {
            close(targetFileFD);
            return RC_ERROR;
        }

        eventHandler.addEvent(EVFILT_READ, targetFileFD, EventContext::EV_GETResponse, &clientConnection);

        clientConnection.initResponseBodyBySize(buf.st_size);

        return RC_IN_PROGRESS;
    }
    const std::string absoluteIndexPath = targetRepresentationURI + "/" + location.getIndex();
    if (stat(absoluteIndexPath.c_str(), &buf) == 0
            && (buf.st_mode & S_IFREG) != 0) {
        this->appendStatusLine(clientConnection, Status::I_200);

        this->appendDefaultHeaderFields(clientConnection);
        clientConnection.appendResponseMessage("Content-Type: ");
        std::string type;
        updateContentType(location.getIndex(), type);
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

        if (buf.st_size == 0)
            return RC_SUCCESS;
        const int targetFileFD = open(absoluteIndexPath.c_str(), O_RDONLY);
        if (targetFileFD == -1)
            return RC_ERROR;
        if (fcntl(targetFileFD, F_SETFL, O_NONBLOCK) == -1) {
            close(targetFileFD);
            return RC_ERROR;
        }

        eventHandler.addEvent(EVFILT_READ, targetFileFD, EventContext::EV_GETResponse, &clientConnection);

        clientConnection.initResponseBodyBySize(buf.st_size);

        return RC_IN_PROGRESS;
    }

    if (location.getAutoIndex()
            && stat(targetRepresentationURI.c_str(), &buf) == 0
            && (buf.st_mode & S_IFDIR) != 0)
        return this->setListResponse(clientConnection, targetRepresentationURI.c_str());

    return this->set404Response(clientConnection);
}


//  event function reading a file and responding of GET request.
//  - Parameters context: context of event.
//  - Return: result of event.
EventContext::EventResult VirtualServer::eventGETResponse(EventContext& context, EventHandler& eventHandler) {
    char buf[BUF_SIZE];
    ssize_t readByteCount;
    const int targetFileFD = context.getIdent();
    Connection& clientConnection = *static_cast<Connection*>(context.getData());

    readByteCount = read(targetFileFD, buf, BUF_SIZE);
    if (readByteCount == -1) {
        delete &context;
        close(targetFileFD);
        return EventContext::ER_Done;
    }

    clientConnection.memcpyResponseMessage(buf, readByteCount);

    if (!clientConnection.isResponseReadAllFile())
        return EventContext::ER_Continue;
    else {
        delete &context;
        close(targetFileFD);

        const int clientSocketFD = clientConnection.getIdent();
        eventHandler.addEvent(EVFILT_WRITE, clientSocketFD, EventContext::EV_Response, &clientConnection);
        return EventContext::ER_Done;
    }
}

//  Process POST request.
//  - Parameters request: The request to process.
//  - Return(None)
VirtualServer::ReturnCode VirtualServer::processPOST(Connection& clientConnection, EventHandler& eventHandler) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    std::string targetRepresentationURI;

    if (this->_others.find("return") != this->_others.end()) {
        if (this->_others.find("return")->second.front().compare("308") == 0)
            return this->set308Response(clientConnection, this->_others);
        return this->set301Response(clientConnection,  this->_others);
    }
    const Location* locationPointer = this->getMatchingLocation(request);
    if (locationPointer == NULL)
        return this->set400Response(clientConnection);
    const Location& location = *locationPointer;
    const std::map<std::string, std::vector<std::string> > &locOthers = location.getOtherDirective();
    if (locOthers.find("return") != locOthers.end()) {
        if (locOthers.find("return")->second.front().compare("308") == 0)
            return this->set308Response(clientConnection, locOthers);
        return this->set301Response(clientConnection, locOthers);
    }
    if (!location.isRequestMethodAllowed(request.getMethod()))
        return this->set405Response(clientConnection, &location);
    int targetClientMaxBodysize = location.getClientMaxBodySize();
    if (targetClientMaxBodysize < 0)
        targetClientMaxBodysize = this->_clientMaxBodySize;
    if (request.getBody().length() > static_cast<std::string::size_type>(targetClientMaxBodysize))
        return this->set413Response(clientConnection);

    location.updateRepresentationPath(targetResourceURI, targetRepresentationURI);

    if (this->detectCGI(clientConnection, location, targetResourceURI) == true) {
        return this->passCGI(clientConnection, location);
    }

    const int targetFileFD = open(targetRepresentationURI.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (targetFileFD == -1)
        return RC_ERROR;
    if (fcntl(targetFileFD, F_SETFL, O_NONBLOCK) == -1) {
        close(targetFileFD);
        return RC_ERROR;
    }

    eventHandler.addEvent(EVFILT_WRITE, targetFileFD, EventContext::EV_POSTResponse, &clientConnection);

    return RC_IN_PROGRESS;
}

//  event function writing a file and responding of POST request.
//  - Parameters context: context of event.
//  - Return: result of event.
EventContext::EventResult VirtualServer::eventPOSTResponse(EventContext& context, EventHandler& eventHandler) {
    typedef std::pair<std::string, const char*> ElementType;
    typedef std::map<int, ElementType> SendBeginMapType;

    static SendBeginMapType sendBeginMap;
    const int targetFileFD = context.getIdent();
    Connection& clientConnection = *static_cast<Connection*>(context.getData());

    ElementType* element;
    const SendBeginMapType::iterator iter = sendBeginMap.find(targetFileFD);
    if (iter == sendBeginMap.end()) {
        element = &sendBeginMap[targetFileFD];
        element->first = clientConnection.getRequest().getBody();
        element->second = &element->first[0];
    }
    else {
        element = &iter->second;
    }

    const char* const sendBegin = element->second;
    std::size_t lengthToSend = std::strlen(sendBegin);

    ssize_t writeByteCount;
    writeByteCount = write(targetFileFD, sendBegin, lengthToSend);

    if (static_cast<std::size_t>(writeByteCount) != lengthToSend) {
        element->second += writeByteCount;
        return EventContext::ER_Continue;
    }

    sendBeginMap.erase(targetFileFD);
    delete &context;
    close(targetFileFD);

    this->appendStatusLine(clientConnection, Status::I_201);

    std::string bodyString;
    this->updateBodyString(Status::I_201, "File created.", bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    const int clientSocketFD = clientConnection.getIdent();
    eventHandler.addEvent(EVFILT_WRITE, clientSocketFD, EventContext::EV_Response, &clientConnection);

    return EventContext::ER_Done;
}

//  Process DELETE request.
//  - Parameters request: The request to process.
//  - Return(None)
VirtualServer::ReturnCode VirtualServer::processDELETE(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();
    const std::string& targetResourceURI = request.getTargetResourceURI();
    struct stat buf;
    std::string targetRepresentationURI;

    if (this->_others.find("return") != this->_others.end()) {
        if (this->_others.find("return")->second.front().compare("308") == 0)
            return this->set308Response(clientConnection, this->_others);
        return this->set301Response(clientConnection,  this->_others);
    }
    const Location* locationPointer = this->getMatchingLocation(request);
    if (locationPointer == NULL)
        return this->set404Response(clientConnection);
    const Location& location = *locationPointer;
    const std::map<std::string, std::vector<std::string> > &locOthers = location.getOtherDirective();
    if (locOthers.find("return") != locOthers.end()) {
        if (locOthers.find("return")->second.front().compare("308") == 0)
            return this->set308Response(clientConnection, locOthers);
        return this->set301Response(clientConnection, locOthers);
    }
    if (!location.isRequestMethodAllowed(request.getMethod()))
        return this->set405Response(clientConnection, &location);
    int targetClientMaxBodysize = location.getClientMaxBodySize();
    if (targetClientMaxBodysize < 0)
        targetClientMaxBodysize = this->_clientMaxBodySize;
    if (request.getBody().length() > static_cast<std::string::size_type>(targetClientMaxBodysize))
        return this->set413Response(clientConnection);

    location.updateRepresentationPath(targetResourceURI, targetRepresentationURI);
    if (stat(targetRepresentationURI.c_str(), &buf) == 0) {

        if (unlink(targetRepresentationURI.c_str()) == -1)
            return RC_ERROR;

        this->appendStatusLine(clientConnection, Status::I_200);

        std::string bodyString;
        this->updateBodyString(Status::I_200, "File deleted.", bodyString);

        this->appendContentDefaultHeaderFields(clientConnection);
        clientConnection.appendResponseMessage("Content-Length: ");
        std::ostringstream oss;
        oss << bodyString.length();
        clientConnection.appendResponseMessage(oss.str());
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
        clientConnection.appendResponseMessage("\r\n");
        clientConnection.appendResponseMessage(bodyString);

        return RC_SUCCESS;
    }

    return this->set404Response(clientConnection);
}

//  Set status line to response of clientConnection.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
void VirtualServer::appendStatusLine(Connection& clientConnection, Status::Index index) {
    clientConnection.appendResponseMessage("HTTP/1.1 ");
    clientConnection.appendResponseMessage(HTTP::getStatusCodeBy(index));
    clientConnection.appendResponseMessage(" ");
    clientConnection.appendResponseMessage(HTTP::getStatusReasonBy(index));
    clientConnection.appendResponseMessage("\r\n");
}

//  append default header fields.
void VirtualServer::appendDefaultHeaderFields(Connection& clientConnection) {
    clientConnection.appendResponseMessage("Server: crash-webserve\r\n");
    clientConnection.appendResponseMessage("Date: ");
    clientConnection.appendResponseMessage(this->makeDateHeaderField());
    clientConnection.appendResponseMessage("\r\n");
}

//  append default header fields for error code.
void VirtualServer::appendContentDefaultHeaderFields(Connection& clientConnection) {
    this->appendDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Type: text/html\r\n");
}

//  update body string.
void VirtualServer::updateBodyString(HTTP::Status::Index index, const char* description, std::string& bodyString) const {
    const char* statusCode = getStatusCodeBy(index);
    const std::map<std::string, std::string>::const_iterator errorPageIterator = this->_errorPage.find(statusCode);
    if (errorPageIterator == this->_errorPage.end())
        ::updateBodyString(index, description, bodyString);
    else
        bodyString = errorPageIterator->second;
}

VirtualServer::ReturnCode VirtualServer::set201Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_201);

    std::string bodyString;
    this->updateBodyString(Status::I_201, "file created", bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    return RC_SUCCESS;
}

VirtualServer::ReturnCode VirtualServer::set301Response(Connection& clientConnection, const std::map<std::string, std::vector<std::string> >& locOther) {
    std::string bodyString;
    std::stringstream ss;

    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_301);
    this->appendDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("Content-Length: ");
    this->updateBodyString(Status::I_301, NULL, bodyString);
    ss << bodyString.size();
    clientConnection.appendResponseMessage(ss.str().c_str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Content-Type: text/html");
    clientConnection.appendResponseMessage("\r\n");
    // location
    clientConnection.appendResponseMessage("Location: ");
    clientConnection.appendResponseMessage(this->makeLocationHeaderField(locOther) + clientConnection.getRequest().getTargetResourceURI().substr(1));
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("\r\n");

    clientConnection.appendResponseMessage(bodyString);
    return RC_SUCCESS;
}

VirtualServer::ReturnCode VirtualServer::set308Response(Connection& clientConnection, const std::map<std::string, std::vector<std::string> >& locOther) {
    std::string bodyString;
    std::stringstream ss;

    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_308);
    this->appendDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("Content-Length: ");
    this->updateBodyString(Status::I_308, NULL, bodyString);
    ss << bodyString.size();
    clientConnection.appendResponseMessage(ss.str().c_str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Content-Type: text/html");
    clientConnection.appendResponseMessage("\r\n");
    // location
    clientConnection.appendResponseMessage("Location: ");
    clientConnection.appendResponseMessage(this->makeLocationHeaderField(locOther) + clientConnection.getRequest().getTargetResourceURI().substr(1));
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("\r\n");

    clientConnection.appendResponseMessage(bodyString);
    return RC_SUCCESS;
}

//  set response message with 400 status.
//  - Parameters clientConnection: The client connection.
//  - Return: upon successful completion a value of 0 is returned.
//      otherwise, a value of -1 is returned.
VirtualServer::ReturnCode VirtualServer::set400Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_400);

    std::string bodyString;
    this->updateBodyString(Status::I_400, NULL, bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    return RC_SUCCESS;
}

//  set response message with 404 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
VirtualServer::ReturnCode VirtualServer::set404Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_404);

    std::string bodyString;
    this->updateBodyString(Status::I_404, NULL, bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    return RC_SUCCESS;
}

//  set response message with 405 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
VirtualServer::ReturnCode VirtualServer::set405Response(Connection& clientConnection, const Location* location) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_405);


    std::string bodyString;
    std::string reqBody = clientConnection.getRequest().getBody();
    this->updateBodyString(Status::I_405, reqBody.c_str(), bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");

    if (location != NULL) {
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
        clientConnection.appendResponseMessage("\r\n");
    }
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    return RC_SUCCESS;
}

//  set response message with 411 status.
//  - Parameters clientConnection: The client connection.
//  - Return: upon successful completion a value of 0 is returned.
//      otherwise, a value of -1 is returned.
VirtualServer::ReturnCode VirtualServer::set411Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_411);

    std::string bodyString;
    this->updateBodyString(Status::I_411, NULL, bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    return RC_SUCCESS;
}

//  set response message with 413 status.
//  - Parameters clientConnection: The client connection.
//  - Return: upon successful completion a value of 0 is returned.
//      otherwise, a value of -1 is returned.
VirtualServer::ReturnCode VirtualServer::set413Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_413);

    std::string bodyString;
    this->updateBodyString(Status::I_413, NULL, bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    return RC_SUCCESS;
}

//  set response message with 500 status.
//  - Parameters clientConnection: The client connection.
//  - Return(None)
VirtualServer::ReturnCode VirtualServer::set500Response(Connection& clientConnection) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_500);

    std::string bodyString;
    this->updateBodyString(Status::I_500, NULL, bodyString);

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << bodyString.length();
    clientConnection.appendResponseMessage(oss.str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage(bodyString);

    return RC_SUCCESS;
}

//  set body for directory listing.
VirtualServer::ReturnCode VirtualServer::setListResponse(Connection& clientConnection, const std::string& path) {
    clientConnection.clearResponseMessage();
    this->appendStatusLine(clientConnection, Status::I_200);

    DIR* dir;

    dir = opendir(path.c_str());
    if (dir == NULL)
        return RC_ERROR;

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

    contentLength += 26;

    this->appendContentDefaultHeaderFields(clientConnection);
    clientConnection.appendResponseMessage("Content-Length: ");
    std::ostringstream oss;
    oss << contentLength;
    clientConnection.appendResponseMessage(oss.str().c_str());
    clientConnection.appendResponseMessage("\r\n");
    clientConnection.appendResponseMessage("Connection: keep-alive\r\n");
    clientConnection.appendResponseMessage("\r\n");

    clientConnection.appendResponseMessage("<html>\r\n<head><title>Index of /</title></head>\r\n<body bgcolor=\"white\">\r\n<h1>Index of /</h1><hr><pre>");

    dir = opendir(path.c_str());
    if (dir == NULL)
        return RC_ERROR;

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

    clientConnection.appendResponseMessage("</pre><hr></body></html>\r\n");

    return RC_SUCCESS;
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

// Make Content-Location Field (path of appropriate data)
//  - Parameters(None)
//  - Return
// std::string VirtualServer::makeContentLocationHeaderField() {
//     // 해당 파일의 경로를 갖고오는 함수
//     // content-type(mime)을 기준으로 찾아주는거 같음
//     // 만약 해당 uri가 파일이다 -> 그대로 출력
//     // 파일인지 모른다 -> accept language랑 accept encoding을 기준으로 해당 경로의 파일들을 다 탐색
//     std::string t;
//     return t;
// }

// Make Location Field (redirection path)
//  - Parameters
//      locOther : etc directive set of connected locations
//  - Return
//      get redirection path. if not find, get null string(TODO)
std::string VirtualServer::makeLocationHeaderField(const std::map<std::string, std::vector<std::string> >& locOther) {
    std::map<std::string, std::vector<std::string> >::const_iterator otherIter = locOther.find("return");
    if (otherIter != locOther.end())
        return otherIter->second.back();
    return ""; // TODO not found redirection path
}

//  set 'type' of 'name'
//  - Parameters
//      name: name of file
//      type: type to set
//  - Return(None)
static void updateContentType(const std::string& name, std::string& type) {
    std::string extension;
    updateExtension(name, extension);
    if (std::strcmp(extension.c_str(), ".txt") == 0)
        type = "text/plain";
    else if (std::strcmp(extension.c_str(), ".html") == 0)
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
    const std::string::size_type extensionBeginPosition = name.rfind('.'); // NOTE : http://localhost/first/test.cgi/bin/var?name=ccc&value=4.5 <- last comma?
    const std::string::size_type extensionEndPosition = name.find_first_of(std::string("/?"), extensionBeginPosition); // NOTE: except fragment
    extension.clear();
    if (extensionBeginPosition !=  std::string::npos)
        extension = name.substr(extensionBeginPosition, extensionEndPosition - extensionBeginPosition);
}

//  generate body string with index, description.
//  - Parameters
//      index: index of status code.
//      description: additional description to display.
//          If you don't need additional description, pass NULL.
//      bodyString: string to store generated body string.
//  - Return(None)
static void updateBodyString(HTTP::Status::Index index, const char* description, std::string& bodyString) {
    const Status& status = Status::_array[index];

    bodyString.clear();
    bodyString += "<html>\r\n<head><title>";
    bodyString += status._statusCode;
    bodyString += ' ';
    bodyString += status._reasonPhrase;
    bodyString += "</title></head>\r\n<body bgcolor=\"white\">\r\n<center><h1>";
    bodyString += status._statusCode;
    bodyString += ' ';
    bodyString += status._reasonPhrase;
    bodyString += "</h1></center>\r\n<hr><center>";
    bodyString += description != NULL ? description : "crash-webserve";
    bodyString += "</center>\r\n</body>\r\n</html>\r\n";
}

// Get value from Request header by the key.
//  - Parameters:
//      request: Request class to get the value.
//      key: searching key.
//  - Return:
//      Value of the key if exists, empty string otherwise.
std::string VirtualServer::getHeaderValue(const Request& request, std::string key) {
    const std::string* value;

    value = request.getFirstHeaderFieldValueByName(key);
    if (value) {
        return *value;
    } else {
        return "";
    }
}

// Just makes inserting code simple to read.
//  - Parameters:
//      key: key.
//      value: matching value.
inline void insertToStringMap(StringMap& envMap, std::string key, std::string value) {
	if (value.empty())
		return ;
    envMap.insert(std::make_pair(key, value));
}
// Add the certain environmental value to _CGIEnvironmentMap
// (For enhancing code readability.)
//  - Parameters:
//      type: from where it getting the environment.
//      key: environment key
//      value: environment value
//  - Return ( None )
void VirtualServer::fillCGIEnvMap(Connection& clientConnection, Location location) {
	StringMap& em = this->_CGIEnvironmentMap;
    const Request& request = clientConnection.getRequest();
    const std::vector<std::string> uriInfo = clientConnection.getRequest().getTargetToken();
    std::string scriptName;

    location.updateRepresentationCGIPath(uriInfo[0], scriptName);
    insertToStringMap(em, "SERVER_SOFTWARE", "FTServer/0.1");
    insertToStringMap(em, "SERVER_NAME", "NoName");
    insertToStringMap(em, "GATEWAY_INTERFACE", "CGI/1.1");

    insertToStringMap(em, "SERVER_PROTOCOL", "HTTP/1.1");
	insertToStringMap(em, "SERVER_PORT", clientConnection.getPortString());
    insertToStringMap(em, "REQUEST_METHOD", request.getMethodString());
    insertToStringMap(em, "PATH_INFO", uriInfo[1].empty() ? "/" : uriInfo[1]);
    insertToStringMap(em, "PATH_TRANSLATED", location.getRoot() + uriInfo[1]);
    insertToStringMap(em, "SCRIPT_NAME", scriptName);
    insertToStringMap(em, "QUERY_STRING", uriInfo[2]);
    insertToStringMap(em, "REMOTE_HOST", "");
    insertToStringMap(em, "REMOTE_ADDR", "");
    insertToStringMap(em, "AUTH_TYPE", this->getHeaderValue(request, "authorization"));
    insertToStringMap(em, "REMOTE_USER", this->getHeaderValue(request, "authorization"));
    insertToStringMap(em, "REMOTE_IDENT", this->getHeaderValue(request, "authorization"));
    insertToStringMap(em, "CONTENT_TYPE", this->getHeaderValue(request, "content-type"));
    insertToStringMap(em, "CONTENT_LENGTH", this->getHeaderValue(request, "content-length"));
}

// Make an envivonments array for CGI Process
//  - Return: envp, the array to pass through 3rd param on execve().
char** VirtualServer::makeCGIEnvironmentArray() {
    char** result;
    std::string	element;


    result = new char*[this->_CGIEnvironmentMap.size() + 1];
	int	idx = 0;
    for (StringMapIter iter = this->_CGIEnvironmentMap.begin();
            iter != this->_CGIEnvironmentMap.end();
            iter++) {
		element = iter->first + "=" + iter->second;
		result[idx] = new char[element.length() + 1];
        strlcpy(result[idx], element.c_str(), element.length() + 1);
        idx++;
	}
	result[idx] = NULL;
	return result;
}

// Make CGI process to handle the query.
//  - Parameters:
//      clientConnection: The Connection object of client who is requesting.
//  - Return ( ReturnCode )
VirtualServer::ReturnCode VirtualServer::passCGI(Connection& clientConnection, const Location& location) {
	pid_t pid;
    char* argScript[2];
	char** envp;
    int pipeToChild[2];
    int pipeFromChild[2];
    std::string cgiPath;
    const std::map<std::string, std::vector<std::string> > & locOther = location.getOtherDirective();
    std::map<std::string, std::vector<std::string> >::const_iterator findResult;
    std::vector<std::string>::iterator directiveCGIPath;

    findResult = locOther.find("cgi_path");
    if (findResult != locOther.end()) {
        cgiPath = findResult->second[0] + _CGIEnvironmentMap["SCRIPT_NAME"];
        argScript[0] = const_cast<char*>(_CGIEnvironmentMap["SCRIPT_NAME"].c_str());
        argScript[1] = NULL;
    } else {
        Log::warning("CGI: cgi_path is not defined.");
        return RC_ERROR;
    }

	try {
		envp = this->makeCGIEnvironmentArray();
	} catch (std::bad_alloc &e) {
		throw std::runtime_error(e.what());
	}

    if (pipe(pipeToChild) == Error || pipe(pipeFromChild) == Error) {
        Log::error("VirtualServer::passCGI pipe() Failed.");
        return RC_ERROR;
    }

    pid = fork();
    if (pid == ChildProcess) {
        close(pipeToChild[1]);
        close(pipeFromChild[0]);
		dup2(pipeToChild[0], STDIN_FILENO);
		dup2(pipeFromChild[1], STDOUT_FILENO);
		execve(cgiPath.c_str(), argScript, const_cast<char*const*>(envp));
        exit(130);
	} else {

        // close(pipeToChild[0]);
        close(pipeFromChild[1]);
        _CGIEnvironmentMap.clear();

        for (size_t i = 0; envp[i]; i++)
            delete[] envp[i];
        delete[] envp;

        if (pid == Error) {
            Log::error("VirtualServer::passCGI fork() Failed.");
            // send response code 500 
            return RC_ERROR;
        }

        int statloc;
        waitpid(pid, &statloc, WNOHANG);
        if (WEXITSTATUS(statloc) == 130) {
            Log::verbose("CGI execution failed.");
        } else {

            clientConnection.addKevent(
                EVFILT_WRITE,
                pipeToChild[1],
                EventContext::EV_CGIParamBody,
                (void*)&clientConnection,
                pipeToChild
            );

            clientConnection.addKevent(
                EVFILT_READ,
                pipeFromChild[0],
                EventContext::EV_CGIResponse,
                (void*)&clientConnection,
                pipeFromChild
            );
        }
    }
    return RC_IN_PROGRESS;
}
