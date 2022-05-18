#include <sys/socket.h>
#include "Response.hpp"

//  Constructor of Response.
Response::Response()
: _sendBegin(NULL) { }

//  Append message to response message.
//  - Parameters
//      message: A message to append.
void Response::appendMessage(const char* message) {
    this->_message += message;
}

//  Send response message to client.
//  - Parameters
//      clientSocket: The socket fd of client
//  - Returns
//      -1: An error has happened.
//      0: All response message has sended.
//      1: Not all response message has sended.
int Response::sendResponseMessage(int clientSocket) {
    if (this->_sendBegin == NULL)
        this->_sendBegin = this->_message.c_str();

    std::size_t lengthToSend = std::strlen(this->_sendBegin);
    ssize_t sendedBytes = send(clientSocket, this->_sendBegin, lengthToSend, 0);

    if (sendedBytes == -1) {
        return -1;
    }
    else if (sendedBytes == lengthToSend) {
        this->_message.clear();
        this->_sendBegin = NULL;

        return 0;
    }
    else {
        this->_sendBegin += sendedBytes;

        return 1;
    }
}
