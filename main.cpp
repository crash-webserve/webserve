#include <sys/event.h>
#include "FTServer.hpp"

int main(int argc, char **argv) {
    FTServer ftServer;
	std::string configFile;

	if (argc != 2) {
		configFile = DEFAULT_CONF_PATH;
		Log::info("Set up default configuration file.");
	} else
		configFile = argv[1];
	try {
        ftServer.initParseConfig(configFile);
		ftServer.init();
		ftServer.run();
	}
	catch (const std::exception& excep) {
		Log::error("VirtualServer::Fatal Error [%s]", excep.what());
		// TODO: graceful shutdown process
	}
    return 0;
}
