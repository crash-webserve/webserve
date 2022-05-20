#ifndef LOCATION_HPP_
#define LOCATION_HPP_

#include <string>
#include <vector>
#include <map>

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
private:
    std::string _route;
    std::string _root;
    std::string _index;
    bool _autoindex;
    char _allowedHTTPMethod;
    std::vector<std::string> _cgiExtension;

    std::map<std::string, std::string> _others;
};

#endif  // LOCATION_HPP_
