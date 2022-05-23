#ifndef RESPONSE_HPP_
#define RESPONSE_HPP_

#include <string>

//  ReturnCaseOfSend indicates the status of send().
//  - Constants
//      RCSEND_ERROR: An error has occured during send().
//      RCSEND_ZERO: The return value of send() was 0.
//          RCSEND_ZERO는 recv()와 달리 큰 의미가 없다고 판단하여 삭제할 예정입니다.
//      RCSEND_SOME: send() transmitted some data, but not all.
//      RCSEND_ALL: send() transmitted all data.
enum ReturnCaseOfSend {
    RCSEND_ERROR = -1,
    RCSEND_ZERO,
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

    void appendMessage(const char* message);
    ReturnCaseOfSend sendResponseMessage(int clientSocket);

private:
    std::string _message;
    const char* _sendBegin;
};

#endif  // RESPONSE_HPP_
