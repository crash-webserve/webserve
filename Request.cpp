#include <sys/socket.h>
#include <sstream>
#include <string>
#include <ctype.h>
#include <cassert>
#include "Request.hpp"
#include "constant.hpp"

//  Destructor of Request object.
Request::~Request() {
    for (HeaderSectionType::iterator iter = this->_headerSection.begin(); iter != this->_headerSection.end(); ++iter)
        delete *iter;
}

//  Returns first header field value of name.
//  - Parameters name: The name to search value.
//  - Return: The first header field value of name
const std::string* Request::getFirstHeaderFieldValueByName(const std::string& name) const {
    for (HeaderSectionType::const_iterator iter = this->_headerSection.begin(); iter != this->_headerSection.end(); ++iter)
        if ((*iter)->first == name)
            return &(*iter)->second;

    return (NULL);
}

//  Receive message from client. If the message is ready to process, parse it.
//  - Parameters clientSocketFD: The fd to recv().
//  - Return: See the type definition.
ReturnCaseOfRecv Request::receive(int clientSocketFD) {
    ssize_t result = this->receiveMessage(clientSocketFD);
    if (result == -1)
        return RCRECV_ERROR;
    else if (result == 0)
        return RCRECV_ZERO;

    if (this->isReadyToProcess()) {
        if (this->parseMessage() == PR_SUCCESS) //  TODO Pass valid string to parseMessage and left remain string.
            return RCRECV_PARSING_SUCCESS;
        else
            return RCRECV_PARSING_FAIL;
    }

    return RCRECV_SOME;
}

//  Receive message from client.
//  - Parameters clientSocketFD: The fd to recv().
//  - Return: the result of recv() call.
ssize_t Request::receiveMessage(int clientSocketFD) {
    char buf[BUF_SIZE];

    ssize_t result = recv(clientSocketFD, buf, BUF_SIZE, 0);
    if (result <= 0)
        return result;

    this->appendMessage(buf);

    return result;
}

//  Append received message from client.
//  - Parameters message: The string of message received from client.
//  - Return(None)
void Request::appendMessage(const char* message) {
    this->_message += message;
}

//  Returns whether Request received the end of header section or not.
//  - Parameters(None)
//  - Return: Whether request received the end of header section or not.
bool Request::isReadyToProcess() const {
    return (this->_message.find("\x0d\x0a\x0d\x0a") != std::string::npos);
}

//  Parse HTTP request message.
//  - Parameters(None)
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseMessage() {
    std::istringstream iss(this->_message);
    std::string line;

    if (!std::getline(iss, line, '\x0d'))
        return PR_FAIL;
    if (this->parseRequestLine(line) == PR_FAIL)
        return PR_FAIL;

    while (true) {
        if (iss.get() == EOF || !iss)
            return PR_FAIL;
        if (!std::getline(iss, line, '\x0d'))
            return PR_FAIL;
        if (line.empty())
            break;
        if (parseHeader(line) == PR_FAIL)
            return PR_FAIL;
    }

    if (iss.get() == EOF || !iss)
        return PR_FAIL;

    this->_body = this->_message.substr(iss.tellg());

    return PR_SUCCESS;
}

//  Parse HTTP request line.
//  - Parameters requestLine: The string of request line.
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseRequestLine(const std::string& requestLine) {
    std::istringstream iss(requestLine);
    std::string token;

    if (iss >> token)
        this->_method = requestMethodByString(token);
    else
        return PR_FAIL;

    if (iss >> token)
        this->_target = token;
    else
        return PR_FAIL;

    if (iss >> token)
        return parseHTTPVersion(token);
    else
        return PR_FAIL;
}

//  Parse HTTP version.
//  - Parameters token: The string of HTTP version.
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseHTTPVersion(const std::string& token) {
    if (token.length() != 8)
        return PR_FAIL;

    if (token.substr(0, 5) != "HTTP/")
        return PR_FAIL;

    if (token[6] != '.')
        return PR_FAIL;

    if (!isdigit(token[5]) || !isdigit(token[7]))
        return PR_FAIL;

    this->_majorVersion = token[5];
    this->_minorVersion = token[7];

    return PR_SUCCESS;
}

//  Parse HTTP message header field.
//  - Parameters headerField: The string of header field.
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseHeader(const std::string& headerField) {
    std::istringstream iss(headerField);
    std::string name;
    std::string value;

    if (!std::getline(iss, name, ':'))
        return PR_FAIL;
    if (name[name.length() - 1] == ' ')
        return PR_FAIL;
    if (!std::getline(iss, value))
        return PR_FAIL;

    this->_headerSection.push_back(new HeaderSectionElementType(name, value));

    return PR_SUCCESS;
}

//  Convert from request method to HTTP::RequestMethod type.
//  - Parameters token: string to convert.
//  - Return: The converted method value.
HTTP::RequestMethod Request::requestMethodByString(const std::string& token) {
    HTTP::RequestMethod method;

    if (token == "GET")
        method = HTTP::RM_GET;
    else if (token == "POST")
        method = HTTP::RM_POST;
    else if (token == "DELETE")
        method = HTTP::RM_DELETE;
    else
        method = HTTP::RM_UNKNOWN;

    return method;
}
