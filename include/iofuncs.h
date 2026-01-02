#ifndef SLASH_IOFUNCS_H
#define SLASH_IOFUNCS_H

#include <unistd.h>
#include "color.h"
#include <string>

namespace io {
  void print(std::string str);
  void print(char c);

  void println(std::string str);
  void error(std::string str);

  void perror(std::string str);
  void debug(std::string str);
}

#endif