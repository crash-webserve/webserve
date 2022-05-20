#include "ServerManager.hpp"
#include "Server.hpp"
#include "Request.hpp"

// default constructor of ServerManager
//  - Parameters(None)
ServerManager::ServerManager() :
_kqueue(-1),
_alive(true)
{
    Log::Verbose("A ServerManager has been generated.");
}

// Destructor of ServerManager
//  - Parameters(None)
ServerManager::~ServerManager() {
    SocketMapIter   sockIter = _mSocket.begin();
    for (;sockIter != _mSocket.end() ; sockIter++) {
        delete sockIter->second;
    }

    for (std::set<ServerConfig *>::iterator itr = _defaultConfigs.begin(); itr != _defaultConfigs.end(); ++itr) {
        delete *itr;
    }

    Log::Verbose("All Sockets has been deleted.");
    // close(_kqueue);
}

// 전달받은 config file을 파싱
//  - TODO
//      - code refactoring
//  - Parameters
//      - filePath: config file이 존재하고 있는 경로
//  - Return(None)
void ServerManager::initParseConfig(std::string filePath) {
    std::fstream        fs;
    std::stringstream   ss;
    std::string         confLine = "";
    std::string         token;
    ServerConfig        *sc;

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
                sc = new ServerConfig();
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
void ServerManager::init() {
    this->initializeServers();
    this->_kqueue = kqueue();

    int     portsOpen[2] = {2000, 2020};
    this->initializeSocket(portsOpen, 2);
}

//  TODO Implement real behavior.
//  Initialize all servers from server config set.
void ServerManager::initializeServers() {
    Server* newServer = new Server(2000, "localhost");
    this->_vServers.push_back(newServer);
}

void ServerManager::initializeSocket(int ports[], int size) {
    Log::Verbose("kqueue generated: ( %d )", _kqueue);
    for (int i = 0; i < size; i++) {
        Socket* newSocket = new Socket(ports[i]);
        _mSocket.insert(std::make_pair(newSocket->getIdent(), newSocket));
        try {
            newSocket->addKevent(_kqueue, EVFILT_READ, NULL);
        } catch(std::exception& exep) {
            Log::Verbose(exep.what());
        }
        Log::Verbose("Socket Generated: [%d]", ports[i]);
    }
}

void ServerManager::clientAccept(Socket* socket) {
    Socket* newSocket = socket->acceptClient();
    _mSocket.insert(std::make_pair(newSocket->getIdent(), newSocket));
    try {
        newSocket->addKevent(_kqueue, EVFILT_READ, NULL);
    } catch(std::exception& excep) {
        Log::Verbose(excep.what());
    }
    Log::Verbose("Client Accepted: [%s]", newSocket->getAddr().c_str());
}

void ServerManager::read(Socket* socket) {
    std::string line;

    socket->receive();

    socket->addReceivedLine(line);
    Server& targetServer = this->getTargetServer(*socket);

    if (socket->getRequest().isReady())
        targetServer.process(*socket, this->_kqueue); 
}

//  TODO Implement real behavior
//  Return appropriate server to process client socket.
//  - Parameters
//      clientSocket: The socket for client.
//  - Returns: Appropriate server to process client socket.
Server& ServerManager::getTargetServer(Socket& clientSocket) {
    (void)clientSocket;
    return *this->_vServers[0];
}

void ServerManager::run() {
    const int MaxEvents = 20;
    struct kevent events[MaxEvents];

    while (_alive == true)
    {
        int numbers = kevent(_kqueue, NULL, 0, events, MaxEvents, NULL);
        if (numbers < 0)
        {
            Log::Verbose("Server::run kevent error [%d]", errno);
            continue;
        }
        Log::Verbose("kevent found %d events.", numbers);
        for (int i = 0; i < numbers; i++)
        {
            // struct kevent& event = events[i];
            int filter = events[i].filter;
            Socket* eventSocket = _mSocket[events[i].ident];

            try
            {
                if (filter == EVFILT_READ && eventSocket->isclient() == false)
                    clientAccept(eventSocket);
                else if (filter == EVFILT_READ)
                    this->read(eventSocket);
                else if (filter == EVFILT_WRITE) {
                    eventSocket->transmit();
                }
            }
            catch (const std::exception& excep)
            {
                Log::Verbose("Server::run Error [%s]", excep.what());
            }
        }
    }
}
