#include <interpreter/interpreter.h>
#include <sys/wait.h>
#include <iofuncs.h>
#include <unistd.h>
#include <interpreter/interpreter.h>
#include <cstring>
#include <session.h>
#include <fstream>

// the type is to get the boolean, and if true, get the command in the shebang at the same time
// if .second is empty, execute script in slash
std::pair<bool, std::string> Interpreter::is_script(std::string file) {
  if(!file.contains("/")) { return {false, ""}; }

  std::ifstream f(file, std::ios::binary);
  if(!f.is_open()) {
    error(runtime::file_access_failed, "failed to open " + file + ": " + strerror(errno));
    return {errno, ""};
  }

  char buffer[4];
  f.read(buffer, 4);

  if(buffer[0] == '\x1f' && buffer[1] == 'E' && buffer[2] == 'L' && buffer[3] == 'F')
    return {false, ""};

  f.seekg(0); // reset pointer to 0 after reading 4 chars 

  // for the shebang
  std::string line;
  while(std::getline(f, line)) {
    if(line.empty()) continue;
    if(line.starts_with("#!")) return {true, line.substr(2)};

    return {true, ""};
  }
}

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
      args.push_back(const_cast<char*>(s.c_str()));
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

  int child_status = 0;
  if (pid == 0) {
      execvp(args[0], args.data());
    
      child_status = errno;
      _exit(errno);
  }
  if(child_status != 0) {
    error(runtime::syscall_failed, "failed to execute " + std::string(args[0]) + ": " + strerror(errno));
    return child_status;
  }

  int status;
  pid_t w = waitpid(pid, &status, 0);
  if(w == -1) {
    if(errno != ECHILD) {
      error(runtime::syscall_failed, "waitpid failed: " + std::string(strerror(errno)));
      return -1;
    }
  }

  int exit_code;
  if(WIFEXITED(status)) exit_code = WEXITSTATUS(status);
  else if(WIFSIGNALED(status)) exit_code = 128 + WTERMSIG(status);
  else exit_code = -1;

  Session::last_exit = exit_code;
  return Status(exit_code);
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