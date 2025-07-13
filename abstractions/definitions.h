#ifndef AMBER_DEFINITIONS_H
#define AMBER_DEFINITIONS_H

#define STDOUT STDOUT_FILENO
#define STDERR STDERR_FILENO
#define STDIN STDIN_FILENO

#include <string_view>

// ANSI colors

const std::string reset     = "\x1b[0m";
const std::string bold      = "\x1b[1m";
const std::string underline = "\x1b[4m";

const std::string black     = "\x1b[30m";
const std::string red       = "\x1b[31m";
const std::string green     = "\x1b[32m";
const std::string yellow    = "\x1b[33m";
const std::string blue      = "\x1b[34m";
const std::string magenta   = "\x1b[35m";
const std::string cyan      = "\x1b[36m";
const std::string white     = "\x1b[37m";

const std::string orange    = "\x1b[38;5;202m";
const std::string gray      = "\x1b[38;5;237m";



#endif //AMBER_DEFINITIONS_H
