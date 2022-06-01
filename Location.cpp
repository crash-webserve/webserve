#include "Location.hpp"

Location::Location():
_route(""),
_root(""),
_index(""),
_autoindex(false),
_allowedHTTPMethod(7)
{
}

//  Set representationPath for resource.(this->_route -> this->_root)
//  - Parameters
//      resourceURI: The resource path to convert to local path.
//      representationPath: The path of representation for resource.
//  - Return(None)
void Location::updateRepresentationPath(const std::string& resourceURI, std::string& representationPath) const {
    representationPath = this->_root + '/';
    representationPath += (resourceURI.c_str() + this->_route.length());
}
