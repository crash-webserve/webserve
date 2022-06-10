#include "VirtualServerConfig.hpp"

//  Constructor of VirtualServerConfig
//  - Parameters(None)
VirtualServerConfig::VirtualServerConfig()
: _inBrace(false){
}

//  Destructor of VirtualServerConfig
//  - Parameters(None)
VirtualServerConfig::~VirtualServerConfig() {
    for (std::set<LocationConfig *>::iterator itr = _locations.begin(); itr != _locations.end(); ++itr)
        delete *itr;
}

//  Append single config block to Config container
//  - Parameters
//      - directive: 
//      - value: 
//  - return(None)
void    VirtualServerConfig::appendConfig(std::string directive, std::vector<std::string> value) {
    _configs.insert(make_pair(directive, value));
}

//  Parse and tokenize raw string to server block
//  - Parameters
//      fs: file stream with configuration file information 
//      ss: string steam for generating tokens in semantic unit
//      confLine: get one line from file stream(fs)
//  - return
//      If parsing success, it's true. if not, false.
//  TODO
//      - Functionalization progresses in semantic units
bool    VirtualServerConfig::parsing(std::fstream &fs, std::stringstream &ss, std::string confLine)
{
    std::string token;
    std::string directive = "";
    std::vector<std::string> values;
    LocationConfig *lc;

    ss >> token;
    if (token != "{")
        return false;
    this->_inBrace = true;
    // token.clear();
    while (true) {
        if (ss.eof()) {
            if (!getline(fs, confLine)) // 스트림이 비어있고 가져올 줄이 없는 경우
                break ;
            ss.clear();
            ss << confLine;
        }
        if (!(ss >> token)) // \t도 인식하는 것 같음(lldb에서는 확인 안됨)
            continue;
        if (token == "location") {
            lc = new LocationConfig();
            if (!lc->parsing(fs, ss, confLine))
                return false;
            ss >> token;
            this->_locations.insert(lc);
        }
        else if (token == "}") {
            this->_inBrace = false;
            break;
        }
        else if (directive.empty())
            directive = token;
        else if (token.substr(token.length() - 1) == ";") {
            values.push_back(token.substr(0, token.length() - 1));
            this->appendConfig(directive, values);
            directive.clear();
            values.clear();
        }
        else
            values.push_back(token);
    }
    return true;
}
