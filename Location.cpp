#include "Location.hpp"
#include "constant.hpp"

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

void Location::updateRepresentationCGIPath(const std::string& resourceURI, std::string& representationCGIPath) const {
        representationCGIPath = (resourceURI.c_str() + this->_route.length());
}

//  get client_max_body_size value.
//  - Parameters(None)
//  - Return: client_max_body_size value in int.
int Location::getClientMaxBodySize() const {
    const std::map<std::string, std::vector<std::string> >::const_iterator iter = this->_others.find("client_max_body_size");
    if (iter != this->_others.end()) {
        std::istringstream iss(iter->second[0]);
        int maxBodySize;
        iss >> maxBodySize;
        if (!iss)
            return -1;
        return maxBodySize;
    }
    return -1;
}
