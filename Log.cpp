#include "Log.hpp"


const char* Log::getPrefix(int logLevel) {
    switch (logLevel)
    {
        case LogVerbose:
            return "\033[34m[VERBOSE]\033[0m";
        case LogDebug:
            return "\033[37m[DEBUG]  \033[0m";
        case LogInfo:
            return "\033[32m[INFO]   \033[0m";
        case LogWarning:
            return "\033[33m[WARNING]\033[0m";
        case LogError:
            return "\033[35m[ERROR]  \033[0m";
        default:
            return "\033[0m";
    }
}

void Log::printPrefixed(int logLevel, const char* format, va_list& va) {
    std::cout << getPrefix(logLevel);
    std::cout << std::setw(20) << std::right << ":: ";
    std::vfprintf(stdout, format, va);
    std::cout << std::endl;
}

#define LOG_PRINT(LEVEL) va_list va; va_start(va, format); printPrefixed(LEVEL, format, va); va_end(va);

#if LOG_LEVEL >= 5
void Log::verbose(const char* format, ...) { LOG_PRINT(LogVerbose) };
#else
void Log::verbose(const char* format, ...) {};
#endif

#if LOG_LEVEL >= 4
void Log::debug(const char* format, ...) { LOG_PRINT(LogDebug) };
#else
void Log::debug(const char* format, ...) {};
#endif

#if LOG_LEVEL >= 3
void Log::info(const char* format, ...) { LOG_PRINT(LogInfo) };
#else
void Log::info(const char* format, ...) {};
#endif

#if LOG_LEVEL >= 2
void Log::warning(const char* format, ...) { LOG_PRINT(LogWarning) };
#else
void Log::warning(const char* format, ...) {};
#endif

#if LOG_LEVEL >= 1
void Log::error(const char* format, ...) { LOG_PRINT(LogError) };
#else
void Log::error(const char* format, ...) {};
#endif
