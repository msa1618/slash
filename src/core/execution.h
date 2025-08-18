#ifndef SLASH_EXECUTION_H
#define SLASH_EXECUTION_H

#include <string>
#include <vector>
#include <chrono>

struct RedirectInfo {
    bool enabled;
    bool stdout_;
    bool append;
    std::string filepath;
};

struct ExecFlags {
  // Used for waiting, stdout and stderr not needed since they're handled in the child and not the parent
  bool exit_code;
  bool repeat;
  bool time;
};

int pipe_execute(std::vector<std::vector<std::string>> commands);
int wait_foreground_job(pid_t pid, const std::string& cmd, ExecFlags flags, std::chrono::_V2::system_clock::time_point start);
int execute(std::vector<std::string> parsed_args, std::string input, bool save_to_history, bool bg, RedirectInfo rinfo, ExecFlags info);
int save_to_history(std::vector<std::string> parsed_arg, std::string input);

int exec(std::vector<std::string> args, std::string raw_input, bool save_to_his);

#endif // SLASH_EXECUTION_H
