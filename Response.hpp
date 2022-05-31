#ifndef RESPONSE_HPP_
#define RESPONSE_HPP_

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include "Log.hpp"

//  ReturnCaseOfSend indicates the status of send().
//  - Constants
//      RCSEND_ERROR: An error has occured during send().
//      RCSEND_SOME: send() transmitted some data, but not all.
//      RCSEND_ALL: send() transmitted all data.
enum ReturnCaseOfSend {
    RCSEND_ERROR = -1,
    RCSEND_SOME,
    RCSEND_ALL,
};

//  ReturnCaseOfSend indicates the status of send().
//  - Constants
//      RCSEND_ERROR: An error has occured during send().
//      RCSEND_SOME: send() transmitted some data, but not all.
//      RCSEND_ALL: send() transmitted all data.
enum ReadResult {
    Error = -1,
    Done,
    Continuing,
};


//  Store response message.
//  If fail sending message at once, this->_sendBegin store the point where to
//  begin sending.
//  - Member variable
//      _message: A message to send.
//      _sendBegin: Begging point to send.
class Response {
public:
    Response();

    ReadResult readCGIresult(int fd);

    void clearMessage();
    void appendMessage(const std::string& message);
    ReturnCaseOfSend sendResponseMessage(int clientSocket);

private:
    std::string _message;
    std::string _cgiResult;
    const char* _sendBegin;

    enum { CGIBufferSize = 1000 };
};

#endif  // RESPONSE_HPP_
