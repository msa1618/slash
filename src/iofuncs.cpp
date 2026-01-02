#include <iofuncs.h>
#include <cstring>

namespace io {
  void print(std::string str) {
    write(STDOUT_FILENO, str.c_str(), str.length());
  }
  void print(char c) {
    write(STDOUT_FILENO, &c, 1);
  }
  void println(std::string str)
  {
    print(str + "\n");
  }
  void error(std::string str) {
    write(STDERR_FILENO, str.c_str(), str.size());
  }
  void perror(std::string str) {
    error(str + ": " + std::string(strerror(errno)) + "\n");
  }
  void debug(std::string str) {
    error(cl::cyan + "debug: " + cl::reset + str + "\n");
  }
}