#ifndef SLASH_IOFUNCS_H
#define SLASH_IOFUNCS_H

#include <unistd.h>
#include <cstdarg>
#include <string>
#include <vector>
#include "color.h"
#include <cstring>
#include <regex>
#include <cerrno>

namespace io {

inline std::string vformat(const char* fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int len = vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    if (len < 0) return {};

    std::vector<char> buf(len + 1);
    vsnprintf(buf.data(), buf.size(), fmt, args);
    return std::string(buf.data(), len);
}

inline void print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string s = vformat(fmt, args);
    va_end(args);
    write(STDOUT_FILENO, s.data(), s.size());
}

inline void println(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string s = vformat(fmt, args);
    va_end(args);
    s.push_back('\n');
    write(STDOUT_FILENO, s.data(), s.size());
}

inline void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string s = vformat(fmt, args);
    va_end(args);
    write(STDERR_FILENO, s.data(), s.size());
}

inline void debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string s = vformat(fmt, args);
    va_end(args);
    std::string out = cl::cyan + "debug: " + cl::reset + s + "\n";
    write(STDERR_FILENO, out.data(), out.size());
}

void perror(std::string str);
std::string strip_ansi(std::string& str);

}

#endif
