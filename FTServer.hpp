#ifndef FTSERVER_HPP_
#define FTSERVER_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <exception>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <utility>
#include "Log.hpp"
#include "VirtualServer.hpp"
#include "Connection.hpp"
#include "VirtualServerConfig.hpp"

// NOTE port, server_name only one
// vector<string>? for multiple server name
struct ServerConfigKey {
    std::string _port;
    std::vector<std::string> _server_name;
    bool operator<(const ServerConfigKey s) const {
        return ((this->_port < s._port) || (this->_server_name < s._server_name));
    }
};

// Manage multiple servers (like nginx)
//  - TODO  
//      config 컨테이너에서 실제 서버 객체 만드는 메소드
//      함수 네이밍 변경(좀 더 구분하기 쉬운 형태로)
//  - Member variables  
//      _defaultConfigs: config file에서 파싱해서 정리한 config 컨테이너(vector), 서버마다 속성값 다르기에 구분
//      _vVirtualServers: 
//      _mConnection
//      _kqueue
//      _alive
//  - Methods
//      init: Read and parse configuration file to initialize server. 
class FTServer {
public:
    FTServer();
    ~FTServer();

    void init();
    void initializeVirtualServers();
    void initParseConfig(std::string configfile);
    void initializeConnection(std::set<port_t>&  ports, int size);

    VirtualServer& getTargetVirtualServer(Connection& connection);
    void run();

private:
    typedef std::vector<VirtualServer*>             VirtualServerVec;
    typedef std::map<int, Connection*>           ConnectionMap;
    typedef std::map<int, Connection*>::iterator ConnectionMapIter;
    typedef std::vector<VirtualServerConfig *>  VirtualServerConfigVec;
    typedef std::vector<VirtualServerConfig *>::iterator  VirtualServerConfigIter;

    VirtualServerConfigVec _defaultConfigs;
    VirtualServerVec       _vVirtualServers;
    ConnectionMap       _mConnection;
    std::map<port_t, VirtualServer*> _defaultVirtualServers;
    int             _kqueue;
    bool            _alive;

    void addDummyVirtualServer();
    VirtualServer* makeVirtualServer(VirtualServerConfig* serverConf);
    void acceptConnection(Connection* connection);
    void closeConnection(int ident);
    void read(Connection* connection);
    void write(Connection* connection);
    VirtualServer* selectVirtualServer(/* Request Object */);
};

#endif  // FTSERVER_HPP_
