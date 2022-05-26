#include <fstream>
#include <cstring>
#include <cassert>
#include "VirtualServer.hpp"
#include "Connection.hpp"
#include "constant.hpp"

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
_name(""),
_connection(nullptr)
{
} 

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
//  - Return: See the type definition.
VirtualServer::ReturnCode VirtualServer::processRequest(Connection& clientConnection) {
    const Request& request = clientConnection.getRequest();

    this->setStatusCode(Status::_array[Status::SI_DEFAULT]._statusCode);
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

    return VirtualServer::RC_SUCCESS;
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

    if (this->isStatusDefault()) {
        this->setStatusCode(Status::_array[Status::SI_NOT_FOUND]._statusCode);
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
        this->setStatusCode(Status::_array[Status::SI_METHOD_NOT_ALLOWED]._statusCode);
        return;
    }

    const std::string& targetResourceURI = request.getTargetResourceURI();
    location.getRepresentationPath(targetResourceURI, this->_targetRepresentationURI);
}

//  Set response message according to this->_statusCode.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
void VirtualServer::setResponseMessageByStatusCode(Connection& clientConnection) {
    if (this->isStatusCode(Status::_array[Status::SI_OK]._statusCode))
        setOKGETResponse(clientConnection);
}

//  Set response message as OK status response of GET request.
//  - Parameters clientConnection: The Connection object of client who is requesting.
//  - Return(None)
int VirtualServer::setOKGETResponse(Connection& clientConnection) {
    clientConnection.appendResponseMessage("HTTP/1.1 ");
    clientConnection.appendResponseMessage(Status::_array[Status::SI_OK]._statusCode);
    clientConnection.appendResponseMessage(Status::_array[Status::SI_OK]._reasonPhrase);
    clientConnection.appendResponseMessage("\r\n");

    // TODO append header section
    clientConnection.appendResponseMessage(clientConnection.makeHeaderField(HTTP::DATE));

    std::ifstream targetRepresentation(this->_targetRepresentationURI, std::ios_base::binary | std::ios_base::ate);
    if (!targetRepresentation.is_open()) {
        setStatusCode(Status::_array[Status::SI_INTERNAL_SERVER_ERROR]._statusCode);
        return -1;
    }

    std::ifstream::pos_type size = targetRepresentation.tellg();
    std::string str(size, '\0');
    targetRepresentation.seekg(0);
    if (targetRepresentation.read(&str[0], size))
        clientConnection.appendResponseMessage(str.c_str());

    return 0;
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
		envp = this->makeCGIEnvironmentArray();
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
		execve(/*CGI PATH*/0, NULL, const_cast<char*const*>(envp));
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

        while (1) {
            int readResult;
            char buffer[CGIBufferSize];

            memset(buffer, 0, CGIBufferSize);
            readResult = read(pipeFromChild[0], buffer, CGIBufferSize - 1);
            cgiOutput += buffer;
            if (readResult == 0) {
                clientConnection.appendResponseMessage(cgiOutput);
                Log::info("CGI has been processed.");
                return ;
            } else if (readResult < 0) {
                Log::info("CGI process error.");
                return ;
            }
        }
        // waitpid(-1, NULL, 0);
    }
}