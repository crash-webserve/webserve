#include "LocationConfig.hpp"

//  Constructor of location config
//  - Parameters(None)
LocationConfig::LocationConfig()
:_inBrace(false),
_path("")
{
    this->_directives.clear();
}

//  Destructor of location config
//  - Parameters(None)
LocationConfig::~LocationConfig()
{
    // TODO : make delete routine(member variables)
}

//  Append new header to header container
//  - Parameters
//      fieldName: header field name
//      fieldValue: header field value(s)
//  - Return(None)
void    LocationConfig::appendHeader(std::string fieldName, std::vector<std::string> fieldValue){
    _directives.insert(make_pair(fieldName, fieldValue));
}

//  Parse and tokenize raw string to location block
//  - Parameters
//      fs: file stream with configuration file information 
//      ss: string steam for generating tokens in semantic unit
//      confLine: get one line from file stream(fs)
//  - return
//      If parsing success, it's true. if not, false.
//  TODO
//      - Functionalization progresses in semantic units
bool    LocationConfig::parsing(std::fstream &fs, std::stringstream &ss, std::string confLine)
{
    std::string token;
    std::string fn;
    std::vector<std::string> fv;

    ss >> token;
    if (token.find_first_of("/") != 0)
        return false;
    this->setPath(token);
    ss >> token;
    if (token != "{")
        return false;
    this->_inBrace = true;
    while (true)
    {
        if (!ss) {
            if (!getline(fs, confLine)) // 스트림이 비어있고 가져올 줄이 없는 경우
                break ;
            ss.clear();
            ss << confLine;
        }
        if (!(ss >> token)) // \t도 인식하는 것 같음(lldb에서는 확인 안됨)
            continue;
        if (token == "}") {
            this->_inBrace = false;
            break;
        }
        else if (fn.empty())
            fn = token;
        else if (token.substr(token.length() - 1) == ";") {
            fv.push_back(token.substr(0, token.length() - 1));
            this->_directives.insert(std::pair<std::string, std::vector<std::string> >(fn, fv));
            fn.clear();
            fv.clear();
        }
        else 
            fv.push_back(token);
    }
    return true;
}