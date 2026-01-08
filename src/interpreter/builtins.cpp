#include <interpreter/interpreter.h>
#include <unistd.h>
#include <getopt.h>
#include <cstring>
#include <unordered_set>

bool Interpreter::is_builtin(std::string cmd) {
  std::unordered_set<std::string> builtins = {"cd", "read", "fc", ":"};

  return builtins.find(cmd) != builtins.end();
}

Interpreter::Status Interpreter::i_builtin(std::string cmd, int argc, char *argv[]) {
  if(cmd == "cd")   return i_cd(argc, argv);
  if(cmd == "read") return i_read(argc, argv);
  if(cmd == "fc")   return i_fc(argc, argv);
  if(cmd == ":")    return Status(0);

  error(runtime::invalid_arguments, "builtin not found: " + cmd);
}

Interpreter::Status Interpreter::i_cd(int argc, char* argv[]) {
  if(argc > 2) {
    error(runtime::invalid_arguments, "too many arguments for cd");
    return Status(1);
  }

  std::string dir;
  if(argc == 1) dir = getenv("HOME");
  else dir = argv[1];
  
  if(dir == "-") {
    if(!getenv("OLDPWD")) {
      error(runtime::invalid_arguments, "OLDPWD is not set");
      return Status(1);
    }
    dir = getenv("OLDPWD");
  }

  if(chdir(dir.c_str()) != 0) {
    error(runtime::syscall_failed, "failed to cd to " + dir + ": " + strerror(errno));
    return Status(errno);
  }

  return 0;
}


Interpreter::Status Interpreter::i_read(int argc, char *argv[])
{
  return Status(1);
}

Interpreter::Status Interpreter::i_fc(int argc, char *argv[])
{
  return Status(1);
}
