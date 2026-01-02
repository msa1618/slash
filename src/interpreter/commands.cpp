#include <interpreter/interpreter.h>
#include <sys/wait.h>
#include <iofuncs.h>
#include <unistd.h>
#include <interpreter/interpreter.h>
#include <cstring>
#include <session.h>

Interpreter::Status Interpreter::i_command(AST::NodeCommand* node) {
  pid_t pid = fork();
  if(pid == -1) {
    error(runtime::syscall_failed, "failed to fork process: " + std::string(strerror(errno)));
    return Status(errno);
  }
  if (pid == 0) {
      std::string program_str = expand(node->program);
      std::vector<std::string> arg_strings;

      arg_strings.reserve(node->arguments.size() + 1);
      arg_strings.push_back(program_str);
      for (auto& e : node->arguments)
          arg_strings.push_back(expand(e));


      std::vector<char*> args;
      args.reserve(arg_strings.size() + 1);
      for (auto& s : arg_strings)
          args.push_back(s.data());
      args.push_back(nullptr);

      execvp(args[0], args.data());
    
      io::perror("failed to execute " + program_str);
      _exit(errno);
  }

  int status;
  waitpid(pid, &status, 0);

  int exit = 0;
  if(WIFEXITED(status)) exit = WEXITSTATUS(status);
  if(WIFSIGNALED(status)) exit = WTERMSIG(status);

  Session::last_exit = exit;
  return Status(exit);
}
Interpreter::Status Interpreter::i_exit(AST::NodeExit* node) {
  int exit = Session::last_exit;
  if(!node->code.empty()) exit = to_int(node->code);

  _exit(exit);
}