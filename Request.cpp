#include <sys/socket.h>
#include <sstream>
#include <string>
#include <cctype>
#include <cassert>
#include "Request.hpp"
#include "constant.hpp"

static void tolower(std::string& value);

Request::Request()
: _parsingStatus(S_NONE)
{ }

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

//  clear message.
//  - Parameter(None)
//  - Return(None)
void Request::clearMessage() {
    this->_message.clear();
}

//  Receive message from client. If the message is ready to process, parse it.
//  - Parameters clientSocketFD: The fd to recv().
//  - Return: See the type definition.
ReturnCaseOfRecv Request::receive(int clientSocketFD) {
    if (!(this->isStatusNone() || this->isStatusParsingBody()))
        return RCRECV_ALREADY_PROCESSING_WAIT;

    ssize_t result = this->receiveMessage(clientSocketFD);
    if (result == -1)
        return RCRECV_ERROR;
    else if (result == 0)
        return RCRECV_ZERO;

    ReturnCaseOfRecv returnCode;
    if (this->isReadyToProcess() || this->isStatusParsingBody()) {
        this->_parsingStatus = this->parseMessage();
        if (this->_parsingStatus == S_PARSING_FAIL)
            this->_message.clear();
        returnCode = (this->_parsingStatus == S_PARSING_BODY) ? RCRECV_SOME : RCRECV_PARSING_FINISH;
    }
    else
        returnCode = RCRECV_SOME;

    return returnCode;
}

//  Returns whether Request received the end of header section or not.
//  - Parameters(None)
//  - Return: Whether request received the end of header section or not.
bool Request::isReadyToProcess() const {
    return (this->_message.find("\r\n\r\n") != std::string::npos);
}

//  Return whether the body is chunked or not.
//  - Parameters(None)
//  - Return: Whether the body is chunked or not.
bool Request::isChunked() const {
    const std::string* headerFieldValue = getFirstHeaderFieldValueByName("transfer-encoding");
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

    ssize_t result = recv(clientSocketFD, buf, BUF_SIZE - 1, 0);
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
Request::Status Request::parseMessage() {
    std::istringstream iss(this->_message);
    std::size_t parsedPositionOfMessage = 0;

    if (!this->isStatusParsingBody()) {
        std::string line;

        if (!std::getline(iss, line, '\r'))
            return S_PARSING_FAIL;
        if (this->parseRequestLine(line) == PR_FAIL)
            return S_PARSING_FAIL;

        for (HeaderSectionType::iterator itr = this->_headerSection.begin();
            itr != this->_headerSection.end();
            itr++) {
                delete *itr;
            }
        this->_headerSection.clear();
        while (true) {
            iss.get();
            if (!std::getline(iss, line, '\r'))
                return S_PARSING_FAIL;
            if (line.empty())
                break;
            if (parseHeader(line) == PR_FAIL)
                return S_PARSING_FAIL;
        }

        if (iss.get() != '\n')
            return S_PARSING_FAIL;

        this->_body.clear();
        parsedPositionOfMessage = iss.tellg();
        this->_parsingStatus = S_PARSING_BODY;
    }

    ParsingResult result;

    if (this->isChunked()) {
        result = this->parseChunkToBody(iss, parsedPositionOfMessage);
    }
    else {
        const std::string* headerFieldValue = getFirstHeaderFieldValueByName("content-length");
        if (headerFieldValue != NULL) {
            std::istringstream sizeStream(*headerFieldValue);
            ssize_t ssizeBodySize;
            sizeStream >> ssizeBodySize;
            if (!sizeStream || sizeStream.get() != EOF || ssizeBodySize < 0)
                return S_PARSING_FAIL;
            const std::size_t bodySize = static_cast<std::size_t>(ssizeBodySize);
            const std::size_t sizeLeft = bodySize - this->_body.length();
            std::string moreBody = this->_message.substr(parsedPositionOfMessage, sizeLeft);
            this->_body += moreBody;
            parsedPositionOfMessage += moreBody.length();
            if (this->_body.length() != bodySize)
                result = PR_EOF;
            else
                result = PR_SUCCESS;
        }
        else {
            if (this->_method == HTTP::RM_POST)
                return S_LENGTH_REQUIRED;
            else
                result = PR_SUCCESS;
        }
    }

    this->_message.erase(0, parsedPositionOfMessage);

    if (result == PR_FAIL)
        return S_PARSING_FAIL;
    else if (result == PR_EOF) {
        return S_PARSING_BODY;
    }

    return S_PARSING_SUCCESS;
}

//  Parse HTTP request line.
//  - Parameters requestLine: The string of request line.
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseRequestLine(const std::string& requestLine) {
    std::istringstream iss(requestLine);
    std::string token;

    iss >> token;
    this->_method = requestMethodByString(token);
    iss >> token;
    this->_target = token;
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
    if (!(ch == '\t' || ch == ' '))
        iss.putback(ch);
    if (!std::getline(iss, value))
        return PR_FAIL;
    ch = value[value.length() - 1];
    if (ch == '\t' || ch == ' ')
        value.erase(value.length() - 1);

    tolower(name);
    this->_headerSection.push_back(new HeaderSectionElementType(name, value)); // leaks

    return PR_SUCCESS;
}

//  Parse chunked body.
//  - Parameters iss: input string stream of body.
//  - Return: Whether the parsing succeeded or not.
ParsingResult Request::parseChunkToBody(std::istringstream& iss, std::size_t& parsedPositionOfMessage) {
    while (true) {
        const std::ios_base::fmtflags ff = iss.flags();
        iss.setf(std::ios_base::hex, std::ios_base::basefield);
        int chunkLength;
        if (std::isspace(iss.peek()))
            return iss.eof() ? PR_EOF : PR_FAIL;
        iss >> chunkLength;
        iss.flags(ff);
        iss.get();
        iss.get();
        std::string chunk(chunkLength + 1, '\0');
        iss.read(&chunk[0], chunkLength);
        if (iss.get() != '\r' || iss.get() != '\n')
            return iss.eof() ? PR_EOF : PR_FAIL;

        this->_body += chunk;
        parsedPositionOfMessage = iss.tellg();

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
    else if (token == "PUT")
        method = HTTP::RM_PUT;
    else
        method = HTTP::RM_UNKNOWN;

    return method;
}

//  make value to lower case string.
//  - Parameters value: the string to make lower case.
//  - Return(None)
static void tolower(std::string& value) {
    for (std::string::iterator iter = value.begin(); iter != value.end(); ++iter)
        *iter = tolower(*iter);
}
