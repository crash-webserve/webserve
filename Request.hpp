#ifndef REQUEST_HPP_
#define REQUEST_HPP_

#include <string>
#include <vector>

//  ParsingResult indicates the result of parsing.
enum ParsingResult {
    PR_SUCCESS,
    PR_FAIL,
};

namespace HTTP {

//  RequestMethod indicates request method type of HTTP request message.
enum RequestMethod {
    RM_UNKNOWN = 0x0,
    RM_GET = 0x1 << 0,
    RM_POST = 0x1 << 1,
    RM_DELETE = 0x1 << 2,
};

enum HeaderFieldName{
    ALLOW,
    AUTHORIZATION,
    WWW_AUTHENTICATE,
    CONTENT_LANGUAGE,
    CONTENT_LENGTH,
    CONTENT_TYPE,
    CONTENT_LOCATION,
    LOCATION,
    DATE,
    HOST,
    LAST_MODIFIED,
    REFERER,
    RETRY_AFTER,
    SERVER,
    TRANSFER_ENCODING,
    USER_AGENT,
};

}   // namespace HTTP

//  ReturnCaseOfRecv indicates the status of recv() call.
//  - Constants
//      RCRECV_ERROR: An error has occured.
//      RCRECV_ZERO: Nothing received.
//      RCRECV_SOME: Received some message but not enough to process.
//      RCRECV_PARSING_FAIL: Received enough message to process, but failed parsing.
//      RCRECV_PARSING_SUCCESS: Received enough message to process, and succeeded parsing.
enum ReturnCaseOfRecv {
    RCRECV_ERROR = -1,
    RCRECV_ZERO,
    RCRECV_SOME,
    RCRECV_PARSING_FAIL,
    RCRECV_PARSING_SUCCESS,
};

//  Accumulate HTTP request message and parse it and store.
//  - member variables
//      _message: Accumulated HTTP request message.
//
//      _method: Parsed request method.
//      _target: Parsed target resource URI.
//      _majorVersion: Parsed major version.
//      _minorVersion: Parsed major version.
//      _headerSection: Parsed header field vector.
//      _body: Parsed payload body.
class Request {
public:
    typedef std::pair<std::string, std::string> HeaderSectionElementType;
    typedef std::vector<HeaderSectionElementType*> HeaderSectionType;

    ~Request();

    HTTP::RequestMethod getMethod() const { return this->_method; };
    const std::string& getTargetResourceURI() const { return this->_target; };
    char getMajorVersion() const { return this->_majorVersion; };
    char getMinorVersion() const { return this->_minorVersion; };
    std::string getMessage() const { return this->_message; };
    const std::string* getFirstHeaderFieldValueByName(const std::string& name) const;
    const std::string& getBody() const { return this->_body; };

    ReturnCaseOfRecv receive(int clientSocketFD);

private:
    std::string _message;

    HTTP::RequestMethod _method;
    std::string _target;
    char _majorVersion;
    char _minorVersion;

    HeaderSectionType _headerSection;

    std::string _body;

    ssize_t receiveMessage(int clientSocketFD);
    void appendMessage(const char* message);
    bool isReadyToProcess() const;

    ParsingResult parseMessage();
    ParsingResult parseRequestLine(const std::string& requestLine);
    ParsingResult parseHTTPVersion(const std::string& token);
    ParsingResult parseHeader(const std::string& headerField);

    HTTP::RequestMethod requestMethodByString(const std::string& token);
};

#endif  // REQUEST_HPP_
