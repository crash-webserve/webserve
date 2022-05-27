#include <sys/socket.h>
#include "Response.hpp"

//  Constructor of Response.
Response::Response()
: _sendBegin(NULL)
, _message("") { }

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
        this->_sendBegin = this->_message.c_str();

    std::size_t lengthToSend = std::strlen(this->_sendBegin);
    ssize_t sendedBytes = send(clientSocket, this->_sendBegin, lengthToSend, 0);

    if (sendedBytes == -1) {
        return RCSEND_ERROR;
    }
    else if (sendedBytes != lengthToSend) {
        this->_sendBegin += sendedBytes;

        return RCSEND_SOME;
    }
    else {
        this->_message.clear();
        this->_sendBegin = NULL;

        return RCSEND_ALL;
    }
}
