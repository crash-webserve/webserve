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
};

// Manage multiple servers (like nginx)
//  - TODO  
//      config 컨테이너에서 실제 서버 객체 만드는 메소드
//      함수 네이밍 변경(좀 더 구분하기 쉬운 형태로)
//  - Member variables  
//      _defaultConfigs: config file에서 파싱해서 정리한 config 컨테이너(set), 서버마다 속성값 다르기에 구분
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
    void initializeConnection(std::set<int>&  ports, int size);

    VirtualServer& getTargetVirtualServer(Connection& connection);
    void run();

private:
    struct compServer
    {
        bool operator()(const ServerConfigKey& lhs, const ServerConfigKey& rhs) const
        {
            return ((lhs._port < rhs._port) || (lhs._server_name < rhs._server_name));
        }
    };
    typedef std::vector<VirtualServer*>             VirtualServerVec;
    typedef std::map<int, Connection*>           ConnectionMap;
    typedef std::map<int, Connection*>::iterator ConnectionMapIter;
    typedef std::map<ServerConfigKey, VirtualServerConfig *, compServer >  VirtualServerConfigMap;
    typedef std::map<ServerConfigKey, VirtualServerConfig *, compServer >::iterator  VirtualServerConfigIter;

    VirtualServerConfigMap _defaultConfigs;
    VirtualServerVec       _vVirtualServers;
    ConnectionMap       _mConnection;
    std::map<int, VirtualServer*> _defaultVirtualServers; // port별 기본 서버
    int             _kqueue;
    bool            _alive;

    VirtualServer* makeVirtualServer(VirtualServerConfig* serverConf);
    void acceptConnection(Connection* connection);
    void closeConnection(int ident);
    void read(Connection* connection);
    void write(Connection* connection);
    VirtualServer* selectVirtualServer(/* Request Object */);
};

#endif  // FTSERVER_HPP_
