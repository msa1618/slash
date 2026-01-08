#include <interpreter/interpreter.h>
#include <sys/wait.h>
#include <iofuncs.h>
#include <unistd.h>
#include <interpreter/interpreter.h>
#include <cstring>
#include <session.h>

Interpreter::Status Interpreter::i_command(AST::NodeCommand* node) {
  // get program name and argv
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
  ////////////////////////////


  if (is_builtin(program_str)) {
    return i_builtin(program_str, arg_strings.size(), args.data());
  }

  pid_t pid = fork();
  if(pid == -1) {
    error(runtime::syscall_failed, "failed to fork process: " + std::string(strerror(errno)));
    return Status(errno);
  }
  if (pid == 0) {
      execvp(args[0], args.data());
    
      error(runtime::syscall_failed, "failed to execute " + std::string(args[0]) + ": " + strerror(errno));
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

Interpreter::Status Interpreter::i_connector(AST::NodeConnector* node) {
  if(node->connector == "||" || node->connector == "&&") {
    Status left = i_statement(node->left);
    if(left.exit != 0 && node->connector == "||") return i_statement(node->right);
    if(left.exit == 0 && node->connector == "&&") return i_statement(node->right);

    return left;
  }

  if(node->connector == "|" || node->connector == "|&") {
    int pipefds[2];
    if(pipe(pipefds) != 0) { error(runtime::syscall_failed, "pipe(): " + std::string(strerror(errno))); return Status(errno); }

    pid_t left = fork();
    if(left == -1) {
       error(runtime::syscall_failed, "fork(): " + std::string(strerror(errno))); return Status(errno);
    }

    if(left == 0) {
      dup2(pipefds[1], STDOUT_FILENO);
      if(node->connector == "|&") dup2(pipefds[1], STDERR_FILENO);
      close(pipefds[0]); close(pipefds[1]);

      _exit(i_statement(node->left).exit);
    }

    pid_t right = fork();
    if(right == -1) { error(runtime::syscall_failed, "pipe(): " + std::string(strerror(errno))); return Status(errno); } // TODO

    if(right == 0) {
      dup2(pipefds[0], STDIN_FILENO);
      close(pipefds[0]); close(pipefds[1]);

      _exit(i_statement(node->right).exit);
    }

    close(pipefds[0]);
    close(pipefds[1]);

    int status1, status2;
    waitpid(left, &status1, 0);
    waitpid(right, &status2, 0);

    Status status(0);
    status.exit = WEXITSTATUS(status2);
    // TODO: handle pipefail when the feature comes

    return status;
  }
}