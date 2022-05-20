#ifndef SERVERMANAGER_HPP_
#define SERVERMANAGER_HPP_

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
#include "Log.hpp"
#include "Server.hpp"
#include "Socket.hpp"
#include "ServerConfig.hpp"

// Manage multiple servers (like nginx)
//  - TODO  
//      config 컨테이너에서 실제 서버 객체 만드는 메소드
//      함수 네이밍 변경(좀 더 구분하기 쉬운 형태로)
//  - Member variables  
//      _defaultConfigs: config file에서 파싱해서 정리한 config 컨테이너(set), 서버마다 속성값 다르기에 구분
//      _vServers: 
//      _mSocket
//      _kqueue
//      _alive
//  - Methods
//      init: Read and parse configuration file to initialize server. 
class ServerManager {
public:
    ServerManager();
    ~ServerManager();

    void init();
    void initializeServers();
    void initParseConfig(std::string configfile);
    void initializeSocket(int ports[], int size);

    Server& getTargetServer(Socket& socket);
    void run();

private:
    typedef std::vector<Server*>             ServerVec;
    typedef std::map<int, Socket*>           SocketMap;
    typedef std::map<int, Socket*>::iterator SocketMapIter;
    typedef std::set<ServerConfig *>         ServerConfigSet;

    ServerConfigSet _defaultConfigs;
    ServerVec _vServers;
    SocketMap _mSocket;
    int _kqueue;
    bool _alive;

    void makeServer(ServerConfig* serverConf);
    void clientAccept(Socket* socket);
    void read(Socket* socket);
    void write(Socket* socket);
    Server* selectServer(/* Request Object */);
};

#endif  // SERVERMANAGER_HPP_
