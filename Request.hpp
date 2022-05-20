#ifndef REQUEST_HPP_
#define REQUEST_HPP_

#include <map>
#include <string>

typedef std::map<std::string, std::string> StringMap;

enum Method {
    METHOD_UNKNOWN,
    METHOD_GET,
    METHOD_POST,
    METHOD_DELETE,
};

//  Accumulate HTTP request message and parse it and store.
//  - member variables
//      _message: Accumulate HTTP request message.
//
//      _method: Parsed request method.
//      _target: Parsed target resource URI.
//      _majorVersion: Parsed major version.
//      _minorVersion: Parsed major version.
//      _headerFieldMap: Parsed header field map.
//      _body: Parsed payload body.
class Request {
public:
    Method getMethod() const { return this->_method; };
    char getMajorVersion() const { return this->_majorVersion; };
    char getMinorVersion() const { return this->_minorVersion; };

    void appendMessage(const std::string& str) { this->_message += str; };
    void parse();

    bool isReady() const;

private:
    std::string _message;

    Method _method;
    std::string _target;
    char _majorVersion;
    char _minorVersion;

    StringMap _headerFieldMap;

    std::string _body;
};

#endif  // REQUEST_HPP_
