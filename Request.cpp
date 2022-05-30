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
        if (this->parseMessage() == PR_SUCCESS)
            return RCRECV_PARSING_SUCCESS;
        else
            return RCRECV_PARSING_FAIL;
    }

    return RCRECV_SOME;
}

//  Returns whether Request received the end of header section or not.
//  - Parameters(None)
//  - Return: Whether request received the end of header section or not.
bool Request::isReadyToProcess() const {
    return (this->_message.find("\x0d\x0a\x0d\x0a") != std::string::npos);
}

//  Return whether the body is chunked or not.
//  - Parameters(None)
//  - Return: Whether the body is chunked or not.
bool Request::isChunked() const {
    const std::string* headerFieldValue = getFirstHeaderFieldValueByName("Transfer-Encoding");
    if (headerFieldValue == NULL)
        return false;

    std::istringstream iss(*headerFieldValue);
    std::string word;
    std::getline(iss, word, ',');
    if (iss.eof())
        return (word == "chunked");
    else
        return false;
}

//  Receive message from client.
//  - Parameters clientSocketFD: The fd to recv().
//  - Return: the result of recv() call.
ssize_t Request::receiveMessage(int clientSocketFD) {
    char buf[BUF_SIZE];

    ssize_t result = recv(clientSocketFD, buf, BUF_SIZE, 0);
    if (result <= 0)
        return result;
    buf[result] = '\0';
    this->appendMessage(buf);

    return result;
}

//  Append received message from client.
//  - Parameters message: The string of message received from client.
//  - Return(None)
void Request::appendMessage(const char* message) {
    this->_message += message;
}

//  Parse HTTP request message.
//  - Parameters(None)
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseMessage() {
    std::istringstream iss(this->_message);
    std::string line;

    if (!std::getline(iss, line, '\r'))
        return PR_FAIL;
    if (this->parseRequestLine(line) == PR_FAIL)
        return PR_FAIL;

    while (true) {
        iss.get();
        if (!std::getline(iss, line, '\r'))
            return PR_FAIL;
        if (line.empty())
            break;
        if (parseHeader(line) == PR_FAIL)
            return PR_FAIL;
    }

    if (iss.get() != '\n')
        return PR_FAIL;

    if (this->isChunked()) {
        if (parseChunkToBody(iss) == PR_FAIL)
            return PR_FAIL;
    }
    else
        this->_body = this->_message.substr(iss.tellg());

    this->_message.clear();

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
    char ch;

    if (!std::getline(iss, name, ':'))
        return PR_FAIL;
    if (name[name.length() - 1] == ' ')
        return PR_FAIL;
    if (!iss.get(ch))
        return PR_FAIL;
    if (!(ch == '\x09' || ch == ' '))
        iss.putback(ch);
    if (!std::getline(iss, value))
        return PR_FAIL;
    ch = value[value.length() - 1];
    if (ch == '\x09' || ch == ' ')
        value.erase(value.length() - 1);

    this->_headerSection.push_back(new HeaderSectionElementType(name, value));

    return PR_SUCCESS;
}

//  Parse chunked body.
//  - Parameters iss: input string stream of body.
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseChunkToBody(std::istringstream& iss) {
    this->_body.clear();
    int contentLength = 0;
    while (true) {
        const std::ios_base::fmtflags ff = iss.flags();
        iss.setf(std::ios_base::hex, std::ios_base::basefield);
        int chunkLength;
        iss >> chunkLength;
        contentLength += chunkLength;
        iss.flags(ff);
        iss.get();
        iss.get();
        if (!iss)
            return PR_FAIL;
        std::string chunk;
        std::getline(iss, chunk, '\r');
        this->_body += chunk;
        if (iss.get() != '\n')
            return PR_FAIL;
        if (chunkLength == 0)
            break;
    }

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
