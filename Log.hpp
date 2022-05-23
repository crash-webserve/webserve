#ifndef LOG_HPP_
#define LOG_HPP_

#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>

#ifndef LOG_LEVEL
#define LOG_LEVEL 5
#endif

class Log {
public:
    static void verbose(const char* format, ...);
    static void debug(const char* format, ...);
    static void info(const char* format, ...);
    static void warning(const char* format, ...);
    static void error(const char* format, ...);

private:
    enum {
        LogVerbose,
        LogDebug,
        LogInfo,
        LogWarning,
        LogError
    };
    static const char* getPrefix(int logLevel);
    static void printPrefixed(int logLevel, const char* format, va_list& va);
    static void printError(int logLevel);
};

#endif
