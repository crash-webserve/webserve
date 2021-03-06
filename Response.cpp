#include "Response.hpp"

//  Constructor of Response.
Response::Response()
: _message("")
, _messageDataSize(0)
, _copyBegin(NULL)
, _sendBegin(NULL) { }

//  clear message.
//  - Parameter(None)
//  - Return(None)
void Response::clearMessage() {
    this->_message.clear();
    this->_sendBegin = NULL;
}

//  Append message to response message.
//  - Parameters
//      message: A message to append.
void Response::appendMessage(const std::string& message) {
    this->_message += message;
}

//  Send response message to client.
//  - Parameters
//      clientSocket: The socket fd of client.
//  - Returns: See the type definition.
ReturnCaseOfSend Response::sendResponseMessage(int clientSocket) {
    if (this->_sendBegin == NULL)
        this->_sendBegin = &this->_message[0];
    if (this->_messageDataSize == 0)
        this->_messageDataSize = this->_message.length();

    const std::string::size_type sendedSize = this->_sendBegin - &this->_message[0];
    const std::string::size_type sizeToSend = this->_messageDataSize - sendedSize;
    ssize_t sendedBytes = send(clientSocket, this->_sendBegin, sizeToSend, 0);

    if (sendedBytes == -1) {
        return RCSEND_ERROR;
    }
    else if (static_cast<std::string::size_type>(sendedBytes) != sizeToSend) {
        this->_sendBegin += sendedBytes;

        return RCSEND_SOME;
    }
    else {
        this->_message.clear();
        this->_sendBegin = NULL;
        this->_messageDataSize = 0;

        return RCSEND_ALL;
    }
}

// initialize body size in Response
void Response::initBodyBySize(std::string::size_type size) {
    std::string::size_type headerSize = this->_message.length();
    this->_messageDataSize = headerSize + size;
    this->_message.reserve(this->_messageDataSize);
    this->_copyBegin = &this->_message[0] + headerSize;
};

void Response::forgeMessageIfEmpty() {
    if (_message.empty() == true) {
        _message = EMPTY_CGI_RESPONSE;
    }
}

void Response::forgeStartlineForCGI() {
    size_t bodyBeginIndex = _message.find("\r\n\r\n") + 4;
    size_t bodyLength;
    size_t findResult;
    std::string contentLengthLine;
    std::ostringstream oss;

    findResult = _message.find("HTTP");
    if (findResult == 0)
        return ;
    _message.insert(0, "HTTP/1.1 200 OK\r\n");
    bodyBeginIndex += 17;
    findResult = _message.find("Content-Length");
    if (findResult != std::string::npos)
        return ;
    // if (findResult > bodyBeginIndex)
    //     return ;
    bodyLength = _message.length() - bodyBeginIndex;
    if (bodyLength < 0)
        return ;
    oss << "Content-Length: " << bodyLength << "\r\n";
    contentLengthLine = oss.str();
    _message.insert(bodyBeginIndex - 2, contentLengthLine);
}
