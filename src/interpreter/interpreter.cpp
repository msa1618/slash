#include <interpreter/interpreter.h>
#include <iofuncs.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/wait.h>

Interpreter::Status Interpreter::execute(AST::Node *node) {
  if(node->is<AST::NodeBlock>()) return i_block(node->as<AST::NodeBlock>());

  if(node->is<AST::NodeExit>())    return i_exit(node->as<AST::NodeExit>());
  if(node->is<AST::NodeCommand>()) return i_command(node->as<AST::NodeCommand>());
  if(node->is<AST::NodeLoopControl>()) return Status(0, node->as<AST::NodeLoopControl>()->is_break ? Signal::Break : Signal::Continue);
  if(node->is<AST::NodeLoop>())    return i_loop(node->as<AST::NodeLoop>());
  if(node->is<AST::NodeConnector>()) return i_connector(node->as<AST::NodeConnector>());
  if(node->is<AST::NodeIf>())      return i_if(node->as<AST::NodeIf>());
  if(node->is<AST::NodeWhile>())   return i_while(node->as<AST::NodeWhile>());
}
void Interpreter::error(runtime run, std::string msg) {
  ++count;
  diagnostics.push_back(Diagnostic(run, msg));
  io::debug("error called");
}
int Interpreter::error_count() {
  return count;
}
int Interpreter::to_int(std::string str) {
  if(str.starts_with("0x") || str.starts_with("0X")) return std::stoi(str.substr(2), nullptr, 16);
  if(str.starts_with("0b") || str.starts_with("0B")) return std::stoi(str.substr(2), nullptr, 2);
  if(str.starts_with("0")) return std::stoi(str.substr(2), nullptr, 8);
  return std::stoi(str, nullptr, 10);
}

decltype(Interpreter::diagnostics) Interpreter::diags() {
  return diagnostics;
}

Interpreter::Status Interpreter::i_block(AST::NodeBlock *node) {
  if(node->subshell) {
    pid_t pid = fork();
    if(pid == -1) { error(runtime::syscall_failed, "failed to fork for subshell: " + std::string(strerror(errno))); return Status(errno); }

    if(pid == 0) {
      for(auto& stmt : node->statements) {
        Status status = i_statement(stmt);
        if(status.signal != Signal::None) { _exit(0); return status; }  
      }
    }

    int status;
    waitpid(pid, &status, 0);
  } else {
    for(auto& stmt : node->statements) {
      Status status = i_statement(stmt);
      if(status.signal != Signal::None) return status;  
    }
  }
  return Status(0);
}

Interpreter::Status Interpreter::i_statement(AST::Node *node) {
    struct SavedFd {
        int original;
        int redirected;
    };
    std::vector<SavedFd> savedfds;

    auto save = [&](int original, int redirected) {
        savedfds.push_back({original, redirected});
    };

    if(node->redirection.input.filename) {
        std::string filename = expand(node->redirection.input.filename);
        int fd = open(filename.c_str(), O_RDONLY);
        if(fd == -1) {
            error(runtime::syscall_failed, "open input: " + filename + ": " + std::string(strerror(errno)));
            return Status(errno);
        }

        int saved = dup(STDIN_FILENO);
        if(saved == -1) {
            close(fd);
            error(runtime::syscall_failed, "dup stdin: " + std::string(strerror(errno)));
            return Status(errno);
        }

        if(dup2(fd, STDIN_FILENO) == -1) {
            close(fd);
            close(saved);
            error(runtime::syscall_failed, "dup2 stdin: " + std::string(strerror(errno)));
            return Status(errno);
        }

        save(saved, STDIN_FILENO);
        close(fd);
    }

    for(auto& r : node->redirection.redirections) {
        std::string filename = expand(r.filename);
        int to_fd = to_int(r.file_descriptor);

        int fd;
        if(!filename.starts_with("&")) fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        else {
          std::string within = filename.substr(1);
          if(within == "-") {
            save(dup(to_fd), to_fd);
            close(to_fd);
            continue;
          }

          fd = to_int(within);
        }
        
        if(fd == -1) {
            error(runtime::syscall_failed, "open output: " + filename + ": " + std::string(strerror(errno)));
            return Status(errno);
        }

        int saved = dup(to_fd);
        if(saved == -1) {
            close(fd);
            error(runtime::syscall_failed, "dup output: " + std::string(strerror(errno)));
            return Status(errno);
        }

        if(dup2(fd, to_fd) == -1) {
            close(fd);
            close(saved);
            error(runtime::syscall_failed, "dup2 output: " + std::string(strerror(errno)));
            return Status(errno);
        }

        save(saved, to_fd);
        close(fd);
    }

    Status status = execute(node);

    for(auto& e : savedfds) {
        if(dup2(e.original, e.redirected) == -1) {
            error(runtime::syscall_failed, "dup2 restore: " + std::string(strerror(errno)));
        }
        close(e.original);
    }

    return status;
}
Scope Interpreter::get_scope() { return scope; }