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
    for (ConnectionMapIter   connectionIter = _mConnection.begin();
        connectionIter != _mConnection.end();
        connectionIter++) {
        delete connectionIter->second;
    }
    for (VirtualServerConfigIter itr = _defaultConfigs.begin();
        itr != _defaultConfigs.end();
        ++itr) {
        delete (*itr);
    }
    for (VirtualServerVec::iterator itr = _vVirtualServers.begin();
        itr != _vVirtualServers.end();
        itr++) {
        delete *itr;
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
                if (!sc->parsing(fs, ss, confLine)) {
                    Log::error("fail parsing config");
                    delete sc;
                    continue;
                }
                tConfigs = sc->getConfigs();
                if (tConfigs.find("server_name") != tConfigs.end())
                    key._server_name.push_back(tConfigs.find("server_name")->second[0]);
                else
                    key._server_name.push_back("");
                if (tConfigs.find("listen") != tConfigs.end())
                    key._port = tConfigs.find("listen")->second[0];
                else {
                    Log::error("not find listen value");
                    delete sc;
                    continue;
                }
                if (!checkDuplicate.insert(key).second) {
                    delete sc;
                    continue;
                }
                this->_defaultConfigs.push_back(sc);
            } else
                Log::error("token and server directive don't match");
            ss.clear();
        }
    }
    else
        throw std::domain_error("not open file");
    this->printParseResult();
}

//  Initialize server manager from server config set.
void FTServer::init() {
    if (_eventHandler.getKqueue() < 0)
        throw std::runtime_error("Kqueue Initiation Failed.");

    this->initializeVirtualServers();

    std::set<port_t>     portsOpen;
    for (VirtualServerVec::iterator itr = this->_vVirtualServers.begin();
        itr != this->_vVirtualServers.end(); itr++) {
        portsOpen.insert((*itr)->getPortNumber());
    }
    this->initializeConnection(portsOpen);
}

//  Initialize all virtual servers from virtual server config set.
void FTServer::initializeVirtualServers() {
    for (VirtualServerConfigIter itr = this->_defaultConfigs.begin(); itr != this->_defaultConfigs.end(); itr++) {
        VirtualServer* newVirtualServer = this->makeVirtualServer(*itr);
        this->_vVirtualServers.push_back(newVirtualServer);
        this->_defaultVirtualServers.insert(std::pair<port_t, VirtualServer *>(newVirtualServer->getPortNumber(), newVirtualServer));
    }
}

//  make virtual server from config.
VirtualServer*    FTServer::makeVirtualServer(VirtualServerConfig* virtualServerConf) {
    VirtualServer* newVirtualServer;
    directiveContainer config = virtualServerConf->getConfigs();           // original config in server Block
    std::set<LocationConfig *> locs = virtualServerConf->getLocations();   // config per location block

    std::stringstream ss;
    std::size_t cmbs;
    if (config["server_name"].empty())
        newVirtualServer = new VirtualServer(static_cast<port_t>(std::atoi(config["listen"].front().c_str())),
                            "");
    else
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
        if (itr->first.compare("error_page") == 0) {
            if (itr->second.size() != 2)
                continue;
            newVirtualServer->updateErrorPage(this->_eventHandler, itr->second[0], itr->second[1]);
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
//  - Parameter
//  - Return(none)
void FTServer::initializeConnection(std::set<port_t>& ports) {
    for (std::set<port_t>::iterator itr = ports.begin(); itr != ports.end(); itr++) {
        Connection* newConnection = new Connection(*itr, _eventHandler);
        this->_mConnection.insert(std::make_pair(newConnection->getIdent(), newConnection));
        try {
            _eventHandler.addEvent(
                EVFILT_READ,
                newConnection->getIdent(),
                EventContext::EV_Accept,
                this
            );
        } catch(std::exception& exep) {
            Log::verbose(exep.what());
        }
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
            newConnection->getIdent(),
            EventContext::EV_Request,
            newConnection
        );
    } catch(std::exception& excep) {
        Log::verbose(excep.what());
    }
    Log::verbose("Client Accepted: [%s]", newConnection->getAddr().c_str());
}

void FTServer::acceptConnection(int ident) {
    return this->acceptConnection(_mConnection[ident]);
}

void FTServer::callVirtualServerMethod(EventContext* context) {
    Connection* connection = static_cast<Connection*>(context->getData());
    VirtualServer& matchingServer = getTargetVirtualServer(*connection);
    connection->setTargetVirtualServer(&matchingServer);
    VirtualServer::ReturnCode result;

    result = matchingServer.processRequest(*connection, this->_eventHandler);
    connection->resetRequestStatus();
    switch (result) {
        case VirtualServer::RC_ERROR:
            break;
        case VirtualServer::RC_SUCCESS:
            _eventHandler.addEvent(
                EVFILT_WRITE,
                context->getIdent(),
                EventContext::EV_Response,
                connection
            );
            break;
        case VirtualServer::RC_IN_PROGRESS:
            break;
        default:
            assert(false);
    }
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
        this->callVirtualServerMethod(context);
		break;
	case EventContext::EV_DisposeConn:
		delete this->_mConnection[event.ident];
		this->_mConnection.erase(event.ident);
	default:
		;
	}
	delete context;
}

//  Return appropriate server to process client connection.
//  - Parameters
//      clientConnection: The connection for client.
//  - Returns: Appropriate server to process client connection.
VirtualServer& FTServer::getTargetVirtualServer(Connection& clientConnection) {
    int cntVirtualServers = this->_vVirtualServers.size();
    const std::string* tHostName = clientConnection.getRequest().getFirstHeaderFieldValueByName("host");
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
    struct kevent events[_eventHandler.getMaxEvent()];
    int numbers = 0;

    while (_alive == true) {
    try {
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
    catch (const std::runtime_error& excep) {
        Log::warning("runtime error: %s", excep.what());
    }
    }
}

// Defines how to handle certain event, depands on EventContext
//  - Parameters
//      context: Eventcontext for triggered event
//      filter: filter for triggered event
//  - Returns
//      Result flag of handled event
EventContext::EventResult FTServer::driveThisEvent(EventContext* context, int filter) {
    Connection* connection = static_cast<Connection*>(context->getData());

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
		if (filter != EVFILT_WRITE)
            return EventContext::ER_NA;
        return connection->handleCGIResponse(context->getIdent());
    case EventContext::EV_SetVirtualServerErrorPage:
        return this->eventSetVirtualServerErrorPage(*context);
    case EventContext::EV_GETResponse:
        return this->eventGETResponse(*context);
    case EventContext::EV_POSTResponse:
        return this->eventPOSTResponse(*context);
	default:
		;
    }
	return EventContext::ER_NA;
}

// Process a single event
//  - Parameters
//      event: event to process
//  - Return ( None )
void FTServer::runEachEvent(struct kevent event) {
    EventContext* context = (EventContext*)event.udata;
	int filter = event.filter;
	int eventResult;

    if (filter == EVFILT_USER)
        return ;

	eventResult = this->driveThisEvent(context, filter);

	switch (eventResult) {
	case EventContext::ER_Done:
	case EventContext::ER_Continue:
		break ;
	case EventContext::ER_NA:
		Log::debug("EventContext is not applicalble. (%d): %s", context->getIdent(), context->getCallerTypeToString().c_str());
	case EventContext::ER_Remove:
		_eventHandler.removeEvent(filter, context);
	}

}

//  event function reading a file and setting error page.
//  - Parameters context: context of event.
//  - Return: result of event.
EventContext::EventResult FTServer::eventSetVirtualServerErrorPage(EventContext& context) {
    std::pair<const std::string&, VirtualServer&>* data = static_cast<std::pair<const std::string&, VirtualServer&>*>(context.getData());
    VirtualServer& virtualServer = data->second;

    return virtualServer.eventSetVirtualServerErrorPage(context);
}

//  event function reading a file and responding of GET request.
//  - Parameters context: context of event.
//  - Return: result of event.
EventContext::EventResult FTServer::eventGETResponse(EventContext& context) {
    Connection* clientConnection = static_cast<Connection*>(context.getData());
    VirtualServer* targetVirtualServer = clientConnection->getTargetVirtualServer();

    return targetVirtualServer->eventGETResponse(context, this->_eventHandler);
}

//  event function writing a file and responding of POST request.
//  - Parameters context: context of event.
//  - Return: result of event.
EventContext::EventResult FTServer::eventPOSTResponse(EventContext& context) {
    Connection* clientConnection = static_cast<Connection*>(context.getData());
    VirtualServer* targetVirtualServer = clientConnection->getTargetVirtualServer();

    return targetVirtualServer->eventPOSTResponse(context, this->_eventHandler);
}
void FTServer::printParseResult() {
    // default 서버
    // serverblock별
    //      기본 설정판
    //      location block별 판
    int i = 1;
    for(VirtualServerConfigIter vscItr = this->_defaultConfigs.begin();
        vscItr != this->_defaultConfigs.end();
        vscItr++) {
            directiveContainer c =(*vscItr)->getConfigs();
            std::cout << "==============   " << i <<  "th Server Config  =============\n";
            std::cout << "==============   Basic Directives   =============\n";
            for (directiveContainer::const_iterator cItr = c.begin();
                cItr!= c.end();
                cItr++) {
                std::cout << std::setw(20) << std::left << cItr->first;
                for (std::vector<std::string>::const_iterator cItr2 = cItr->second.begin();
                    cItr2 != cItr->second.end();
                    cItr2++)
                    std::cout << *cItr2 << ' ';
                std::cout << std::endl;
            }
            std::cout << std::endl;

            std::set<LocationConfig *> l =(*vscItr)->getLocations();
            std::cout << "===============     Locations      ===============\n";
            for (std::set<LocationConfig *>::const_iterator lIter = l.begin();
                lIter != l.end();
                lIter++) {
                directiveContainer c = (*lIter)->getDirectives();
                std::cout << "==============  Location Directives ==============\n";
                std::cout << std::setw(20) << std::left << "path ";
                std::cout << (*lIter)->getPath() << '\n';
                for (directiveContainer::const_iterator cItr = c.begin();
                cItr!= c.end();
                cItr++) {
                std::cout << std::setw(20) << std::left  << cItr->first;
                for (std::vector<std::string>::const_iterator cItr2 = cItr->second.begin();
                    cItr2 != cItr->second.end();
                    cItr2++)
                    std::cout << *cItr2 << ' ';
                std::cout << std::endl;
            }
            }
            std::cout << "\n****************************************************\n"  << std::endl;
            i++;
        }
}