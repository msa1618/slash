#ifndef SLASH_COLOR_H
#define SLASH_COLOR_H

#include <string>

namespace cl {
  constexpr std::string reset   = "\033[0m";
  constexpr std::string red     = "\033[31m";
  constexpr std::string green   = "\033[32m";
  constexpr std::string yellow  = "\033[33m";
  constexpr std::string blue    = "\033[34m";
  constexpr std::string magenta = "\033[35m";
  constexpr std::string cyan    = "\033[36m";
  constexpr std::string white   = "\033[37m";
}

#endif