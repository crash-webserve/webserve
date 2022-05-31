#include <fstream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <sys/stat.h>
#include <dirent.h>
#include "VirtualServer.hpp"
#include "Connection.hpp"
#include "EventContext.hpp"
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
                && (buf.st_mode & S_IFREG) != 0) {
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
                && (buf.st_mode & S_IFREG) != 0) {
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

        if (location.getAutoIndex()
                && stat(targetRepresentationURI.c_str(), &buf) == 0
                && (buf.st_mode & S_IFDIR) != 0)
            return this->setListResponse(clientConnection, targetRepresentationURI.c_str());

        break;
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
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));
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

// Get value from Request header by the key.
//  - Parameters:
//      request: Request class to get the value.
//      key: searching key.
//  - Return:
//      Value of the key if exists, empty string otherwise.
std::string VirtualServer::getHeaderValue(Request request, std::string key) {
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
void VirtualServer::insertCGIEnvMap(std::string key, std::string value) {
    this->_CGIEnvironmentMap.insert(std::make_pair(key, value));
}
// Add the certain environmental value to _CGIEnvironmentMap
// (For enhancing code readability.)
//  - Parameters:
//      type: from where it getting the environment.
//      key: environment key
//      value: environment value
//  - Return ( None )
void VirtualServer::fillCGIEnvMap(Connection& clientConnection) {
    Request request;
    request = clientConnection.getRequest();

    this->insertCGIEnvMap("SERVER_SOFTWARE", "FTServer/0.1");
    this->insertCGIEnvMap("SERVER_NAME", "NoName");
    this->insertCGIEnvMap("GATEWAY_INTERFACE", "CGI/1.1");

    this->insertCGIEnvMap("SERVER_PROTOCOL", "HTTP/1.1");
    this->insertCGIEnvMap("SERVER_PORT", clientConnection.getPort(""));
    this->insertCGIEnvMap("REQUEST_METHOD", request.getMethod(""));
    this->insertCGIEnvMap("PATH_INFO", "path to CGI");
    // _location에서 cgi로케이션 구분 하려면??
    this->insertCGIEnvMap("PATH_TRANSLATED", "physical path to CGI");
    this->insertCGIEnvMap("SCRIPT_NAME", "script requested");
    this->insertCGIEnvMap("QUERY_STRING", "query string");
    this->insertCGIEnvMap("REMOTE_HOST", "Hostname of server");
    this->insertCGIEnvMap("REMOTE_ADDR", "IP address");
    // server의 ip를 받아오려면?
    this->insertCGIEnvMap("AUTH_TYPE", this->getHeaderValue(request, "Authorization"));
    this->insertCGIEnvMap("REMOTE_USER", this->getHeaderValue(request, "Authorization"));
    this->insertCGIEnvMap("REMOTE_IDENT", this->getHeaderValue(request, "Authorization"));
    this->insertCGIEnvMap("CONTENT_TYPE", this->getHeaderValue(request, "Content-Type"));
    this->insertCGIEnvMap("CONTENT_LENGTH", this->getHeaderValue(request, "Content-Length"));
}

// Make an envivonments array for CGI Process
//  - Return: envp, the array to pass through 3rd param on execve().
char** VirtualServer::makeCGIEnvironmentArray(Connection& clientConnection) {
    char** result;
    std::string	element;

    this->fillCGIEnvMap(clientConnection);

    result = new char*[this->_CGIEnvironmentMap.size() + 1];
	int	idx = 0;
    for (StringMapIter iter = this->_CGIEnvironmentMap.begin();
            iter != this->_CGIEnvironmentMap.end();
            iter++) {
		element = iter->first + "=" + iter->second;
		result[idx] = new char[element.length() + 1];
        strlcpy(result[idx], element.c_str(), element.length());
        idx++;
	}
	result[idx] = NULL;
	return result;
}

// Make CGI process to handle the query.
//  - Parameters:
//      clientConnection: The Connection object of client who is requesting.
//      kqueueFD: perhaps it may use kevent....
//  - Return ( None )
void VirtualServer::passCGI(Connection& clientConnection, int kqueueFD) {
	pid_t pid;
	char** envp;
	std::string	cgiOutput = "";
    int pipeToChild[2];
    int pipeFromChild[2];

	try {
		envp = this->makeCGIEnvironmentArray(clientConnection);
	} catch (std::bad_alloc &e) {
		std::cerr << e.what() << std::endl;
	}

    if (!pipe(pipeToChild) || !pipe(pipeFromChild)) {
        Log::error("VirtualServer::passCGI pipe() Failed.");
        return ;
    }

    pid = fork();
    if (pid == ChildProcess) {
		dup2(pipeToChild[0], STDIN_FILENO);
		dup2(pipeFromChild[1], STDOUT_FILENO);
		execve("path to script", NULL, const_cast<char*const*>(envp));
		write(STDOUT_FILENO, "Status: 500\r\n\r\n", 15);
        Log::error("VirtualServer::passCGI execve() Failed.");
	} else if (pid == ForkError) {
        Log::error("VirtualServer::passCGI fork() Failed.");
        // send response code 500 
        return ;
    } else {
        close(pipeToChild[0]);
        close(pipeFromChild[1]);

        while (1) {
            size_t writeResult = 0;
            size_t leftSize; // body size
            unsigned long offset = 0;

            writeResult = write(pipeToChild[1], /*body + offset*/0, /*body size*/0);
            if (writeResult == leftSize) {
                break ;
            } else if (writeResult < 0) {
                Log::warning("CGI body pass failed.");
                break ;
            }
            leftSize -= writeResult;
            offset += writeResult;
        }

        for (size_t i = 0; envp[i]; i++)
            delete[] envp[i];
        delete[] envp;

        clientConnection.addKevent(
            EVFILT_READ,
            new EventContext(
                pipeFromChild[0],
                EventContext::CGIResponse,
                (void*)&clientConnection.getResponse())
            );
        // waitpid(-1, NULL, 0);
    }
}
