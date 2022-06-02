#include <cassert>
#include "FTServer.hpp"
#include "VirtualServer.hpp"
#include "Request.hpp"

// default constructor of FTServer
//  - Parameters(None)
FTServer::FTServer() :
_alive(true),
_eventHandler(EventHandler()) {
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
    if (_eventHandler.getKqueue() < 0)
        throw std::runtime_error("Kqueue Initiation Failed.");
    Log::verbose("kqueue generated: ( %d )", _eventHandler.getKqueue());

    this->initializeVirtualServers();

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
    for (std::set<port_t>::iterator itr = ports.begin(); itr != ports.end(); itr++) {
        Connection* newConnection = new Connection(*itr, _eventHandler);
        this->_mConnection.insert(std::make_pair(newConnection->getIdent(), newConnection));
        try {
            _eventHandler.addEvent(
                EVFILT_READ,
                new EventContext(newConnection->getIdent(), EventContext::EV_Accept, this)
            );
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
        _eventHandler.addEvent(
            EVFILT_READ,
            new EventContext(newConnection->getIdent(), EventContext::EV_Request, newConnection)
        );
    } catch(std::exception& excep) {
        Log::verbose(excep.what());
    }
    Log::verbose("Client Accepted: [%s]", newConnection->getAddr().c_str());
}
void FTServer::acceptConnection(int ident) {
    return this->acceptConnection(_mConnection[ident]);
}

// Close the certain socket and destroy the instance.
//  - Parameter
//      ident: socket FD number to kill.
//  - Return(none)
void FTServer::handleUserFlaggedEvent(struct kevent event) {
	EventContext* context = (EventContext*)event.udata;

    if (event.filter != EVFILT_USER)
        return;
	switch (context->getCallerType()) {
	case EventContext::EV_SetVirtualServer:
		*(VirtualServer**)context->getData() = &getTargetVirtualServer(*_mConnection[context->getIdent()]);
		break;
	case EventContext::EV_DisposeConn:
		delete this->_mConnection[event.ident];
		this->_mConnection.erase(event.ident);
	default:
		;
	}
	delete context;

}

//  Read and process requested by client.
//  - Parameters connection: The connection to read from.
//  - Return(None)
void FTServer::read(Connection* connection) {
}

//  Return appropriate server to process client connection.
//  - Parameters
//      clientConnection: The connection for client.
//  - Returns: Appropriate server to process client connection.
VirtualServer& FTServer::getTargetVirtualServer(Connection& clientConnection) {
    //  TODO Implement real behavior. Change the return type from reference to pointer type.
    int cntVirtualServers = this->_vVirtualServers.size();
    const std::string* tHostName = clientConnection.getRequest().getFirstHeaderFieldValueByName("Host");
    for (int i = 0; i < cntVirtualServers; i++) {
        if ((this->_vVirtualServers[i]->getPortNumber() == clientConnection.getPort()) &&
            (this->_vVirtualServers[i]->getServerName() == *tHostName))
            return *this->_vVirtualServers[i];
    }
    return *this->_defaultVirtualServers[static_cast<port_t>(clientConnection.getPort())];
}

// Main loop procedure of ServerManager.
// Do multiflexing job using Kqueue.
//  - Return(none)
void FTServer::run() {
    struct kevent events[_eventHandler.getMaxEvent()];
    int numbers = 0;

    while (_alive == true)
    {
        numbers = _eventHandler.checkEvent(events);
        if (numbers < 0) {
            Log::verbose("VirtualServer::run kevent error [%d]", errno);
            continue;
        }
        for (int i = 0; i < numbers; i++) {
            this->handleUserFlaggedEvent(events[i]);
            this->runEachEvent(events[i]);
        }
    }
}

EventContext::EventResult FTServer::driveThisEvent(EventContext* context, int filter) {
	Connection* connection = (Connection*)context->getData();

	switch (context->getCallerType()) {
	case EventContext::EV_Accept:
		if (filter != EVFILT_READ)
			return EventContext::ER_NA;
		this->acceptConnection(context->getIdent());
		return EventContext::ER_Continue;
	case EventContext::EV_Request:
		if (filter != EVFILT_READ)
			return EventContext::ER_NA;
		return connection->receive();
	case EventContext::EV_Response:
		if (filter != EVFILT_WRITE)
			return EventContext::ER_NA;
		return connection->transmit();
	case EventContext::EV_CGIResponse:
		;
	default:
		;
	}
	return EventContext::ER_NA;

}

void FTServer::runEachEvent(struct kevent event) {
    EventContext* context = (EventContext*)event.udata;
	int filter = event.filter;
	int eventResult;

	eventResult = this->driveThisEvent(context, filter);

	switch (eventResult) {
	case EventContext::ER_Done:
	case EventContext::ER_Continue:
		break ;
	case EventContext::ER_NA:
		Log::debug("EventContext is not applicalble. (%d)", context->getIdent());
	case EventContext::ER_Remove:
		_eventHandler.removeEvent(filter, context);
	}

}
