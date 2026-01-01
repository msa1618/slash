#include <iofuncs.h>

namespace io {
  void print(std::string str) {
    write(STDOUT_FILENO, str.c_str(), str.length());
  }
  void println(std::string str) {
    print(str + "\n");
  }
  void error(std::string str)
  {
    write(STDERR_FILENO, str.c_str(), str.size());
  }
}