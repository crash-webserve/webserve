#ifndef VIRTUALSERVERCONFIG_HPP_
#define VIRTUALSERVERCONFIG_HPP_

#include <set>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include "LocationConfig.hpp"

typedef std::map<std::string, std::vector<std::string> > directiveContainer;
//  Virtual server config block (parsed config file)
//  - Member variables
//      _inBrace: To check if it's inside server block 
//      _locations: having multiple location configurations
//      _configs: uniq configs(directive - values pair) in server block
class VirtualServerConfig {
public:
    VirtualServerConfig();
    ~VirtualServerConfig();
    
    bool    getInBrace() { return this->_inBrace; }
    std::set<LocationConfig *> getLocations() { return this->_locations; }
    std::map<std::string, std::vector<std::string> > getConfigs() { return this->_configs; }
    
    bool    parsing(std::fstream &fs, std::stringstream &ss, std::string confLine);
    void    appendConfig(std::string directive,  std::vector<std::string> value);

private:
    bool                               _inBrace;
    std::set<LocationConfig *>          _locations;
    std::map<std::string, std::vector<std::string> >  _configs;
};
#endif  // VIRTUALSERVERCONFIG_HPP_
