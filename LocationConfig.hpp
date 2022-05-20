#ifndef LOCATIONCONFIG_HPP_
#define LOCATIONCONFIG_HPP_

#include <string>
#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

//  Location config block (parsed config file)
//  - Member variables
//      _inBrace: To check if it's inside location block 
//      _path: path in URL(must start '/')
//      _directives: pair(directive - values) in location block
class LocationConfig {
public:
    LocationConfig();
    ~LocationConfig();

    std::string getPath() { return this->_path;}
    std::map<std::string, std::vector<std::string> > getDirectives() { return this->_directives; }
    void    setPath(std::string path) {this->_path = path;}
    void    setDirectives(std::map<std::string, std::vector<std::string> > directives) {this->_directives = directives;}
    
    void    appendHeader(std::string fieldName, std::vector<std::string> fieldValue);
    bool    parsing(std::fstream &fs, std::stringstream &ss, std::string confLine);

private:
    bool                _inBrace;
    std::string         _path;
    std::map<std::string, std::vector<std::string> > _directives;
};

#endif // LOCATIONCONFIG_HPP_