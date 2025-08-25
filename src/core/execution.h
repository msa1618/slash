#ifndef SLASH_EXECUTION_H
#define SLASH_EXECUTION_H

#include <string>
#include <vector>
#include <chrono>

struct RedirectInfo {
    bool stdout_enabled;
    bool stderr_enabled;

    bool stdout_;
    bool stderr_;

    bool stdout_append;
    bool stderr_append;

    std::string stdout_filepath;
    std::string stderr_filepath;

    bool out_to_err;
    bool out_to_in;
    bool err_to_out;
    bool err_to_in;
    bool in_to_out;
    bool in_to_err;
};

struct ExecFlags {
  // Used for waiting, stdout and stderr not needed since they're handled in the child and not the parent
  bool exit_code;
  bool repeat;
  bool time;
};

int execute(std::vector<std::string> parsed_args, std::string input, bool save_to_history, bool bg, RedirectInfo rinfo, ExecFlags info);
int pipe_execute(std::vector<std::vector<std::string>> commands);
int wait_foreground_job(pid_t pid, const std::string& cmd, ExecFlags flags, bool time, std::chrono::_V2::system_clock::time_point start);
int save_to_history(std::vector<std::string> parsed_arg, std::string input);

int exec(std::vector<std::string> args, std::string raw_input);
int pipe_exec(std::vector<std::vector<std::string>> args, std::string raw_input);

#endif // SLASH_EXECUTION_H
