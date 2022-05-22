#include <sys/event.h>
#include "FTServer.hpp"

int main(int argc, char **argv) {
    FTServer ftServer;

    if (argc != 2)
        std::cerr << "입력 인자 숫자 달라~\n";
    else
        ftServer.initParseConfig(argv[1]);

    ftServer.init();
    ftServer.run();

    return 0;
}
