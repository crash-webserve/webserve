#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <map>

#include <arpa/inet.h>  // inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // socket(), accept(), listen(), bind(), connect(), recv()
#include <sys/time.h>   // struct timespec
#include <fcntl.h>  // fcntl()
#include <sys/event.h>  // struct kevent, kevent()
#include <unistd.h>

#include "Request.hpp"

using std::cin;
using std::cout;
using std::endl;

// constant
const int NORMAL = 0;
const int ERROR = -1;

void Request::parse() {
    std::stringstream ss(this->message);
    std::string token;

    // TODO extraction error if not, abort might occur
    ss >> token;
    initMethod(this->_method, token);

    ss >> token;
    this->_request_target = token;

    ss >> token;
    if (initVersion(this->_major_version, this->_minor_version, token) == ERROR)
        cout << "[error occured]" << endl;

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

    this->_body = this->message.substr(ss.tellg());
}

Request::Method Request::getMethod() const {
    return this->_method;
}

char Request::getMajorVersion() const {
    return this->_major_version;
}

char Request::getMinorVersion() const {
    return this->_minor_version;
}

// TODO implement real behavior
bool Request::isReadyToService() {
    return true;
}

void Request::describe(std::ostream& out) const {
    out << "_method: [" << this->_method << "]" << endl;
    out << "_request_target: [" << this->_request_target << "]" << endl;
    out << "_major_version: [" << this->_major_version << "]" << endl;
    out << "_minor_version: [" << this->_minor_version << "]" << endl;
    out << endl;

    out << "_headerFieldMap:" << endl;
    describeStringMap(out, this->_headerFieldMap);
    out << endl;

    out << "_body: [" << this->_body << "]" << endl;
}