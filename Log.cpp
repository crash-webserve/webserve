#include "Log.hpp"

static void PrintfPrefixed(const char* prefix, const char* format, va_list& va) {
    std::cout << prefix;
    std::cout << std::setw(20) << std::right << ":: ";
    std::vfprintf(stdout, format, va);
    std::cout << std::endl;

    // std::cout << "[";
    // print_time();
    // std::cout << "] ";
    // std::cout << std::setw(5 + 9) << std::left << GetPrefix(level);
    // std::cout << " [";
    // std::cout << std::setw(24) << std::left << prefix;
    // std::cout << "] ";
    // std::vfprintf(stdout, format, va);
    // std::fflush(stdout);
    // std::cout << GetPrefix(-1);
    // std::cout << std::endl;
}

void Log::Verbose(const char* format, ...) {
    va_list va;
    va_start(va, format);
    PrintfPrefixed("[Vervose]", format, va);
    va_end(va);
}
