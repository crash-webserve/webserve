#ifndef LOG_HPP_
#define LOG_HPP_

#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>

#ifndef LOG_LEVEL
#define LOG_LEVEL 5
#endif

//  Parsed value of the HTTP Headers.
class Log {
public:
    static void Verbose(const char* format, ...);
private:
};


#endif
