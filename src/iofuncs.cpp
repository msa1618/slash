#include <iofuncs.h>
#include <cstring>
#include <regex>

namespace io {
  void perror(std::string str) {
    io::error("%s: %s\n", str, strerror(errno)); 
  }

  std::string strip_ansi(std::string& text) {
    std::regex ansi_regex("\x1B\\[[0-?]*[ -/]*[@-~]");
    return std::regex_replace(text, ansi_regex, "");
  }
}