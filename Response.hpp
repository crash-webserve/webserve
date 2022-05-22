#ifndef RESPONSE_HPP_
#define RESPONSE_HPP_

#include <string>

//  Store response message.
//  If fail sending message at once, this->_sendBegin store the point where to
//  begin sending.
//  - Member variable
//      _message: A message to send.
//      _sendBegin: Begging point to send.
class Response {
public:
    Response();

    void appendMessage(const char* message);
    int sendResponseMessage(int clientSocket);

private:
    std::string _message;
    const char* _sendBegin;
};

#endif  // RESPONSE_HPP_
