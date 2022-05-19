#ifndef LOCATION_HPP_
#define LOCATION_HPP_

#include <string>
#include <vector>
#include <map>
#include "Request.hpp"

//  The location directive data for Server.
//  - Member variables
//      _route: The route to match with target resource URI.
//      _root: The path to replace matching _route in target resource URI.
//      _index: The default file to answer if the request is a directory
//      _autoindex: The toggle whether turn on or off directory listing.
//      _allowedHTTPMethod: The bit flags of accepted HTTP methods for the route.
//      _cgiExtension: The file extension of CGI to call.
//      _others: Variable for additional data.
//          std::string _HTTPRedirection: The URI to redirect.
class Location {
public:
    bool isPathMatch(const std::string& resourceURI) const;
    bool isRequestMethodAllowed(HTTP::RequestMethod requestMethod) const;
    void getRepresentationPath(const std::string& resourceURI, std::string& representationPath) const;

private:
    std::string _route;
    std::string _root;
    std::string _index;
    bool _autoindex;
    char _allowedHTTPMethod;
    std::vector<std::string> _cgiExtension;

    std::map<std::string, std::string> _others;
};

//  Return whether the resource path is match to this->_route.
//  - Parameters resourceURI: The path of target resource in request message.
//  - Return: Whether the resource path is match to this->_route.
inline bool Location::isPathMatch(const std::string& resourceURI) const {
    return (resourceURI.compare(0, this->_route.length(), this->_route) == 0);
}

//  Return whether the 'requestMethod' is allowed in this location.
//  - Parameters requestMethod: request method of request.
//  - Return: whether the 'requestMethod' is allowed in this location.
inline bool Location::isRequestMethodAllowed(HTTP::RequestMethod requestMethod) const {
    return (this->_allowedHTTPMethod | requestMethod) != 0;
}

#endif  // LOCATION_HPP_
