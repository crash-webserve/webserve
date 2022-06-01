#include <cassert>
#include "FTServer.hpp"
#include "VirtualServer.hpp"
#include "Request.hpp"

// default constructor of FTServer
//  - Parameters(None)
FTServer::FTServer() :
_kqueue(-1),
_alive(true)
{
    Log::verbose("A FTServer has been generated.");
}

// Destructor of FTServer
//  - Parameters(None)
FTServer::~FTServer() {
    ConnectionMapIter   connectionIter = _mConnection.begin();
    for (;connectionIter != _mConnection.end() ; connectionIter++) {
        delete connectionIter->second;
    }

    for (VirtualServerConfigIter itr = _defaultConfigs.begin(); itr != _defaultConfigs.end(); ++itr) {
        delete (*itr);
    }

    Log::verbose("All Connections has been deleted.");
    // close(_kqueue);
}

// 전달받은 config file을 파싱
//  - TODO
//      - code refactoring
//  - Parameters
//      - filePath: config file이 존재하고 있는 경로
//  - Return(None)
void FTServer::initParseConfig(std::string filePath) {
    std::fstream        fs;
    std::stringstream   ss;
    std::string         confLine = "";
    std::string         token;
    VirtualServerConfig        *sc;
    std::set<ServerConfigKey> checkDuplicate;
    
    fs.open(filePath);
    if (fs.is_open()) {
        while (getline(fs, confLine)) {
            ss << confLine;
            if (!(ss >> token))
            {
                ss.clear();
                continue;
            }
            else if (token == "server") {
                ServerConfigKey key;
                directiveContainer tConfigs;
                sc = new VirtualServerConfig();
                if (!sc->parsing(fs, ss, confLine)) // fstream, stringstream를 전달해주는 방식으로 진행
                    std::cerr << "not parsing config\n"; // 각 요소별 동적할당 해제시켜주는게 중요
                tConfigs = sc->getConfigs();
                if (tConfigs.find("server_name") != tConfigs.end())
                    key._server_name.push_back(tConfigs.find("server_name")->second[0]); // 가장 먼저 입력된 server_name 1개만
                else
                    key._server_name.push_back(""); // TODO server_name 비어있는 경우 "" 설정
                if (tConfigs.find("listen") != tConfigs.end())
                    key._port = tConfigs.find("listen")->second[0];
                else {
                    std::cerr << "not find listen value\n";
                    delete sc;
                    continue;
                }
                if (!checkDuplicate.insert(key).second) {
                    delete sc;
                    continue;
                }
                this->_defaultConfigs.push_back(sc);
            } else
                std::cerr << "not match (token != server)\n"; // (TODO) 오류터졌을 때 동적할당 해제 해줘야함
            ss.clear();
        }
    }
    else
        std::cerr << "not open file\n";
}

//  TODO Implement real behavior.
//  Initialize server manager from server config set.
void FTServer::init() {
    this->initializeVirtualServers();
    this->_kqueue = kqueue();

    std::set<port_t>     portsOpen;
    for (VirtualServerVec::iterator itr = this->_vVirtualServers.begin();
        itr != this->_vVirtualServers.end(); itr++) {
        portsOpen.insert((*itr)->getPortNumber());
    }
    this->initializeConnection(portsOpen, portsOpen.size());
}

//  TODO Implement real behavior.
//  Initialize all virtual servers from virtual server config set.
void FTServer::initializeVirtualServers() {
    for (VirtualServerConfigIter itr = this->_defaultConfigs.begin(); itr != this->_defaultConfigs.end(); itr++) {
        VirtualServer* newVirtualServer = this->makeVirtualServer(*itr);
        this->_vVirtualServers.push_back(newVirtualServer);
        this->_defaultVirtualServers.insert(std::pair<port_t, VirtualServer *>(newVirtualServer->getPortNumber(), newVirtualServer));
    }
}

//  TODO Implement real one virtual server using config
VirtualServer*    FTServer::makeVirtualServer(VirtualServerConfig* virtualServerConf) {
    VirtualServer* newVirtualServer;
    directiveContainer config = virtualServerConf->getConfigs();           // original config in server Block
    std::set<LocationConfig *> locs = virtualServerConf->getLocations();   // config per location block

    std::stringstream ss;
    std::size_t cmbs;

    newVirtualServer = new VirtualServer(static_cast<port_t>(std::atoi(config["listen"].front().c_str())),
                            config["server_name"].front());

    for (directiveContainer::iterator itr = config.begin(); itr != config.end(); itr++) {
        if (!itr->first.compare("listen") || !itr->first.compare("server_name"))
            continue;
        if (!itr->first.compare("client_max_body_size")) {
            ss << itr->second.front();
            ss >> cmbs;
            newVirtualServer->setClientMaxBodySize(cmbs);
            ss.clear();
        }
        else {
            for (size_t i = 0; i < itr->second.size(); i++)
                newVirtualServer->setOtherDirective(itr->first, itr->second);
        }
    }

    for (std::set<LocationConfig *>::iterator itr = locs.begin(); itr != locs.end(); itr++) {
        directiveContainer lcDirect = (*itr)->getDirectives();
        Location* newLocation = new Location();

        // 고유 키 등록
        newLocation->setRoute((*itr)->getPath());
        for (directiveContainer::iterator itr2 = lcDirect.begin(); itr2 != lcDirect.end(); itr2++) {
            if (!itr2->first.compare("autoindex") && !itr2->second.front().compare("on"))
                newLocation->setAutoIndex(true);
            else if (!itr2->first.compare("index")) {
                newLocation->setIndex(itr2->second);
            }
            else if (!itr2->first.compare("allow_method")) {
                newLocation->setAllowedHTTPMethod(itr2->second); 
            }
            else if (!itr2->first.compare("cgi")) {
                newLocation->setCGIExtention(itr2->second);
            }
            else if (!itr2->first.compare("root")) {
                newLocation->setRoot(itr2->second[0]);
            }
            else {
                for (size_t i = 0; i < itr2->second.size(); i++)
                    newLocation->setOtherDirective(itr2->first, itr2->second);
            }
        }
        newVirtualServer->appendLocation(newLocation);
    }
    return newVirtualServer;
}

// Prepares sockets as descripted by the server configuration.
// TODO: make it works with actuall server config!
//  - Parameter
//  - Return(none)
void FTServer::initializeConnection(std::set<port_t>& ports, int size) {
    Log::verbose("kqueue generated: ( %d )", this->_kqueue);
    for (std::set<port_t>::iterator itr = ports.begin(); itr != ports.end(); itr++) {
        Connection* newConnection = new Connection(*itr);
        this->_mConnection.insert(std::make_pair(newConnection->getIdent(), newConnection));
        try {
            newConnection->addKevent(this->_kqueue, EVFILT_READ, NULL);
        } catch(std::exception& exep) {
            Log::verbose(exep.what());
        }
        Log::verbose("Connection Generated: [%d]", (*itr));
    }
}

// Accept the client from the server socket.
//  - Parameter
//      socket: server socket which made a handshake with the incoming client.
//  - Return(none)
void FTServer::acceptConnection(Connection* connection) {
    Connection* newConnection = connection->acceptClient();
    this->_mConnection.insert(std::make_pair(newConnection->getIdent(), newConnection));
    try {
        newConnection->addKevent(this->_kqueue, EVFILT_READ, NULL);
    } catch(std::exception& excep) {
        Log::verbose(excep.what());
    }
    Log::verbose("Client Accepted: [%s]", newConnection->getAddr().c_str());
}

// Close the certain socket and destroy the instance.
//  - Parameter
//      ident: socket FD number to kill.
//  - Return(none)
void FTServer::closeConnection(int ident) {
    delete this->_mConnection[ident];
    this->_mConnection.erase(ident);
}

//  Read and process requested by client.
//  - Parameters connection: The connection to read from.
//  - Return(None)
void FTServer::read(Connection* connection) {
    ReturnCaseOfRecv result = connection->receive();

    switch (result) {
        case RCRECV_ERROR:
            //  TODO Implement behavior
        case RCRECV_ZERO:
            connection->dispose();
            break;
        case RCRECV_SOME:
            break;
        case RCRECV_PARSING_FAIL:
            //  TODO Implement behavior
            break;
        case RCRECV_PARSING_SUCCESS:
            VirtualServer& targetVirtualServer = this->getTargetVirtualServer(*connection);
            if (targetVirtualServer.processRequest(*connection) == VirtualServer::RC_SUCCESS)
                connection->addKevent(this->_kqueue, EVFILT_WRITE, NULL);
            break;
    }
}

//  Return appropriate server to process client connection.
//  - Parameters
//      clientConnection: The connection for client.
//  - Returns: Appropriate server to process client connection.
VirtualServer& FTServer::getTargetVirtualServer(Connection& clientConnection) {
    //  TODO Implement real behavior. Change the return type from reference to pointer type.
    int cntVirtualServers = this->_vVirtualServers.size();
    const std::string* tHostName = clientConnection.getRequest().getFirstHeaderFieldValueByName("Host");
    if (tHostName != NULL) {
        std::string tServerName = tHostName->substr(0, tHostName->find_first_of(":"));
        for (int i = 0; i < cntVirtualServers; i++) {
            if ((this->_vVirtualServers[i]->getPortNumber() == clientConnection.getPort()) &&
                (this->_vVirtualServers[i]->getServerName() == tServerName))
                return *this->_vVirtualServers[i];
        }
    }
    return *this->_defaultVirtualServers[static_cast<port_t>(clientConnection.getPort())];
}

// Main loop procedure of ServerManager.
// Do multiflexing job using Kqueue.
//  - Return(none)
void FTServer::run() {
    const int MaxEvents = 20;
    struct kevent events[MaxEvents];

    while (_alive == true)
    {
        int numbers = kevent(this->_kqueue, NULL, 0, events, MaxEvents, NULL);
        if (numbers < 0)
        {
            Log::verbose("VirtualServer::run kevent error [%d]", errno);
            continue;
        }
        Log::verbose("kevent found %d events.", numbers);
        for (int i = 0; i < numbers; i++)
        {
            // struct kevent& event = events[i];
            int filter = events[i].filter;
            Connection* eventConnection = _mConnection[events[i].ident];

            try
            {
                if (filter == EVFILT_READ && eventConnection->isclient() == false) {
                    this->acceptConnection(eventConnection);
                } else if (filter == EVFILT_READ) {
                    this->read(eventConnection);
                } else if (filter == EVFILT_WRITE) {
                    eventConnection->transmit();
                } else if (filter == EVFILT_USER) {
                    this->closeConnection(events[i].ident);
                }
            }
            catch (const std::exception& excep)
            {
                Log::verbose("VirtualServer::run Error [%s]", excep.what());
            }
        }
    }
}
