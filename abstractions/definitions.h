#ifndef AMBER_DEFINITIONS_H
#define AMBER_DEFINITIONS_H

#define STDOUT STDOUT_FILENO
#define STDERR STDERR_FILENO
#define STDIN STDIN_FILENO

#include <string>

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

// Background colors

const std::string bg_black   = "\x1b[40m";
const std::string bg_red     = "\x1b[41m";
const std::string bg_green   = "\x1b[42m";
const std::string bg_yellow  = "\x1b[43m";
const std::string bg_blue    = "\x1b[44m";
const std::string bg_magenta = "\x1b[45m";
const std::string bg_cyan    = "\x1b[46m";
const std::string bg_white   = "\x1b[47m";

const std::string bg_orange  = "\x1b[48;5;202m";
const std::string bg_gray    = "\x1b[48;5;237m";



#endif //AMBER_DEFINITIONS_H
