#ifndef RESPONSE_HPP_
#define RESPONSE_HPP_

#include <sys/socket.h>
#include <string>

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

//  Store response message.
//  If fail sending message at once, this->_sendBegin store the point where to
//  begin sending.
//  - Member variable
//      _message: A message to send.
//      _sendBegin: Begging point to send.
class Response {
public:
    Response();

    void clearMessage();
    void appendMessage(const std::string& message);

    ReturnCaseOfSend sendResponseMessage(int clientSocket);
    void processCGIResponse();

    void initBodyBySize(std::string::size_type size);
    void memcpyMessage(char* buf, ssize_t size) { memcpy(this->_copyBegin, buf, size); this->_copyBegin += size; };
    bool isReadAllFile() const { return static_cast<std::string::size_type>(this->_copyBegin - &this->_message[0]) == this->_messageDataSize; };

private:
    std::string _message;
    std::string::size_type _messageDataSize;
    char* _copyBegin;
    const char* _sendBegin;
};

#endif  // RESPONSE_HPP_
