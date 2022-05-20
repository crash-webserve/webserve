#include <sstream>
#include <string>
#include <ctype.h>
#include <cassert>
#include "Request.hpp"

// constant
const int NORMAL = 0;
const int ERROR = -1;

static void initMethod(Method& method, const std::string& token);
static int initVersion(char& majorVersion, char& minorVersion, const std::string& token);

//  TODO extraction error if not, abort might occur.
//  Parse HTTP request message.
void Request::parse() {
    std::stringstream ss(this->_message);
    std::string token;

    ss >> token;
    initMethod(this->_method, token);

    ss >> token;
    this->_target = token;

    ss >> token;
    initVersion(this->_majorVersion, this->_minorVersion, token);

    char ch;
    while (isspace(ch = ss.get()));
    ss.putback(ch);
    while (std::getline(ss, token, (char)0x0d)) {
        if (token.length() == 0) {
            ss.get();
            break;
        }
        std::string::size_type colon_position = token.find(':');
        const std::string key = token.substr(0, colon_position);
        const std::string value = token.substr(colon_position + 2);
        this->_headerFieldMap[key] = value;
        ss.get();
    }

    if (ss.eof())
        return;
    else
        ss.clear();

    this->_body = this->_message.substr(ss.tellg());
}

//  TODO implement real behavior.
//  Returns whether Request recieved end of header section or not.
bool Request::isReady() const {
    return true;
}

//  TODO Move to Request::initMethod().
//  Convert from request method to Request::Method type.
//  - Parameters
//      method: reference of type Request::Method to write according to token.
//      token: string to convert to Request::Method type.
static void initMethod(Method& method, const std::string& token) {
    if (token == "GET")
        method = METHOD_GET;
    else if (token == "POST")
        method = METHOD_POST;
    else if (token == "DELETE")
        method = METHOD_DELETE;
    else
        assert(false);
}

//  TODO Move to Request::initVersion().
//  Convert HTTP to Request::Method type.
//  - Parameters
//      majorVersion: reference of type char to save HTTP major version.
//      minorVersion: reference of type char to save HTTP minor version.
//      token: HTTP version token to convert to majorVersion and minorVersion.
static int initVersion(char& majorVersion, char& minorVersion, const std::string& token) {
    if (token.length() != 8)
        return ERROR;

    if (token.substr(0, 5) != "HTTP/")
        return ERROR;

    if (token[6] != '.')
        return ERROR;

    if (!isdigit(token[5]) || !isdigit(token[7]))
        return ERROR;

    majorVersion = token[5];
    minorVersion = token[7];

    return NORMAL;
}
