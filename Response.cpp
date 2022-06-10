#include "Response.hpp"

//  Constructor of Response.
Response::Response()
: _message("")
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

        return RCSEND_ALL;
    }
}

#include <iostream>
void Response::processCGIResponse() {
    std::cout << "---response processing procedure---" << std::endl;
}

void Response::initBodyBySize(std::string::size_type size) {
    std::string::size_type headerSize = this->_message.length();
    this->_messageDataSize = headerSize + size;
    this->_message.reserve(this->_messageDataSize);
    this->_copyBegin = &this->_message[0] + headerSize;
};
