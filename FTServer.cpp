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
    Log::Verbose("A FTServer has been generated.");
}

// Destructor of FTServer
//  - Parameters(None)
FTServer::~FTServer() {
    ConnectionMapIter   connectionIter = _mConnection.begin();
    for (;connectionIter != _mConnection.end() ; connectionIter++) {
        delete connectionIter->second;
    }

    for (std::set<VirtualServerConfig *>::iterator itr = _defaultConfigs.begin(); itr != _defaultConfigs.end(); ++itr) {
        delete *itr;
    }

    Log::Verbose("All Connections has been deleted.");
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
                sc = new VirtualServerConfig();
                if(!sc->parsing(fs, ss, confLine)) // fstream, stringstream를 전달해주는 방식으로 진행
                    std::cerr << "not parsing config\n"; // 각 요소별 동적할당 해제시켜주는게 중요
                this->_defaultConfigs.insert(sc);
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

    int     portsOpen[2] = {2000, 2020};
    this->initializeConnection(portsOpen, 2);
}

//  TODO Implement real behavior.
//  Initialize all servers from server config set.
void FTServer::initializeVirtualServers() {
    VirtualServer* newVirtualServer = new VirtualServer(2000, "localhost");
    this->_vVirtualServers.push_back(newVirtualServer);
}

void FTServer::initializeConnection(int ports[], int size) {
    Log::Verbose("kqueue generated: ( %d )", _kqueue);
    for (int i = 0; i < size; i++) {
        Connection* newConnection = new Connection(ports[i]);
        _mConnection.insert(std::make_pair(newConnection->getIdent(), newConnection));
        try {
            newConnection->addKevent(_kqueue, EVFILT_READ, NULL);
        } catch(std::exception& exep) {
            Log::Verbose(exep.what());
        }
        Log::Verbose("Connection Generated: [%d]", ports[i]);
    }
}

void FTServer::clientAccept(Connection* connection) {
    Connection* newConnection = connection->acceptClient();
    _mConnection.insert(std::make_pair(newConnection->getIdent(), newConnection));
    try {
        newConnection->addKevent(_kqueue, EVFILT_READ, NULL);
    } catch(std::exception& excep) {
        Log::Verbose(excep.what());
    }
    Log::Verbose("Client Accepted: [%s]", newConnection->getAddr().c_str());
}

//  Read and process requested by client.
//  - Parameters connection: The connection to read from.
//  - Return(None)
void FTServer::read(Connection* connection) {
    ReturnCaseOfRecv result = connection->receive();

    switch (result) {
        case RCRECV_ERROR:
            //  TODO Implement behavior
            break;
        case RCRECV_ZERO:
            //  TODO Implement behavior
            break;
        case RCRECV_SOME:
            break;
        case RCRECV_PARSING_FAIL:
            //  TODO Implement behavior
            break;
        case RCRECV_PARSING_SUCCESS:
            VirtualServer& targetVirtualServer = this->getTargetVirtualServer(*connection);
            targetVirtualServer.process(*connection, this->_kqueue);
            break;
        defaut:
            assert(false);
            break;
    }
}

//  Return appropriate server to process client connection.
//  - Parameters
//      clientConnection: The connection for client.
//  - Returns: Appropriate server to process client connection.
VirtualServer& FTServer::getTargetVirtualServer(Connection& clientConnection) {
    //  TODO Implement real behavior. Change the return type from reference to pointer type.
    (void)clientConnection;
    return *this->_vVirtualServers[0];
}

void FTServer::run() {
    const int MaxEvents = 20;
    struct kevent events[MaxEvents];

    while (_alive == true)
    {
        int numbers = kevent(_kqueue, NULL, 0, events, MaxEvents, NULL);
        if (numbers < 0)
        {
            Log::Verbose("VirtualServer::run kevent error [%d]", errno);
            continue;
        }
        Log::Verbose("kevent found %d events.", numbers);
        for (int i = 0; i < numbers; i++)
        {
            // struct kevent& event = events[i];
            int filter = events[i].filter;
            Connection* eventConnection = _mConnection[events[i].ident];

            try
            {
                if (filter == EVFILT_READ && eventConnection->isclient() == false)
                    clientAccept(eventConnection);
                else if (filter == EVFILT_READ)
                    this->read(eventConnection);
                else if (filter == EVFILT_WRITE) {
                    eventConnection->transmit();
                }
            }
            catch (const std::exception& excep)
            {
                Log::Verbose("VirtualServer::run Error [%s]", excep.what());
            }
        }
    }
}
