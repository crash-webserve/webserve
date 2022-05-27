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
    Location();
    bool isRouteMatch(const std::string& resourceURI) const;
    bool isRequestMethodAllowed(HTTP::RequestMethod requestMethod) const;
    void getRepresentationPath(const std::string& resourceURI, std::string& representationPath) const;

    std::string getRoute() { return this->_route; };
    std::string getRoot() { return this->_root; };
    std::string getIndex() { return this->_index; };
    bool getAutoIndex() { return this->_autoindex; };
    char getAllowedHTTPMethod() { return this->_allowedHTTPMethod; };
    std::vector<std::string> getCGIExtention() { return this->_cgiExtension; }

    void setRoute(std::string route) { this->_route = route; };
    void setRoot(std::string root) { this->_root = root; };
    void setIndex(std::vector<std::string> idx) { 
        this->_index = idx[0];  // TODO multiple index
    };
    void setAutoIndex(bool isAutoindex) { this->_autoindex = isAutoindex; };
    void setAllowedHTTPMethod(std::vector<std::string> allowedMethod) {
        if (allowedMethod[0] == "GET")
            this->_allowedHTTPMethod |= HTTP::RM_GET; // TODO multiple method, check RM_GET name
    };
    void setCGIExtention(std::vector<std::string> cgiExt) { this->_cgiExtension = cgiExt; }
    
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
inline bool Location::isRouteMatch(const std::string& resourceURI) const {
    return (resourceURI.compare(0, this->_route.length(), this->_route) == 0);
}

//  Return whether the 'requestMethod' is allowed in this location.
//  - Parameters requestMethod: request method of request.
//  - Return: whether the 'requestMethod' is allowed in this location.
inline bool Location::isRequestMethodAllowed(HTTP::RequestMethod requestMethod) const {
    return (this->_allowedHTTPMethod | requestMethod) != 0;
}

#endif  // LOCATION_HPP_
