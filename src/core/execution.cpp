#include "execution.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../builtin-cmds/cd.h"
#include "../builtin-cmds/var.h"
#include "../builtin-cmds/alias.h"
#include "startup.h"
#include <nlohmann/json.hpp>
#include <thread>
#include "jobs.h"
#include "parser.h"
#include "../builtin-cmds/slash-greeting.h"
#include "../builtin-cmds/help.h"
#include "../builtin-cmds/jobs.h"
#include "cnf.h"
#include <algorithm>

#pragma region helpers

bool is_print_exit_code_enabled() {
  bool pec = false;
  auto settings = io::read_file(slash_dir + "/config/settings.json");
  nlohmann::json json;

  if(std::holds_alternative<std::string>(settings)) {
    json = nlohmann::json::parse(std::get<std::string>(settings));
  } else {
    std::stringstream error;
    error << "Failed to open settings.json: " << strerror(errno);
    info::error(error.str(), errno, "settings.json");
    return false;
  }

  if(json.contains("printExitCodeWhenProgramExits")) {
     pec = json["printExitCodeWhenProgramExits"];
  } else {
    info::error("Missing \"printExitCodeWhenProgramExits\" property in settings");
    return false;
  }

  if(!pec) return false;

  return pec;
}

int save_to_history(std::vector<std::string> parsed_arg, std::string input) {
  std::string cmd = io::split(input, " ")[0];
  std::string full_path = getenv("HOME") + std::string("/.slash/slash-utils/") + cmd;
  if(access(full_path.c_str(), X_OK) == 0) {
    cmd = full_path + cmd;
    parsed_arg[0] = cmd;
  }

  input = io::trim(input);
  std::string history_path = getenv("HOME") + std::string("/.slash/.slash_history");

  if(!io::write_to_file(history_path, input + "\n")) {
    return errno;
  } else return 0;
}

std::string get_signal_name(int signal) {
    static const std::unordered_map<int, std::string> signal_names = {
        {SIGHUP, "SIGHUP"},
        {SIGINT, "SIGINT"},
        {SIGQUIT, "SIGQUIT"},
        {SIGILL, "SIGILL"},
        {SIGTRAP, "SIGTRAP"},
        {SIGABRT, "SIGABRT"},
    #ifdef SIGIOT
        {SIGIOT, "SIGIOT"},
    #endif
        {SIGBUS, "SIGBUS"},
        {SIGFPE, "SIGFPE"},
        {SIGKILL, "SIGKILL"},
        {SIGUSR1, "SIGUSR1"},
        {SIGSEGV, "SIGSEGV"},
        {SIGUSR2, "SIGUSR2"},
        {SIGPIPE, "SIGPIPE"},
        {SIGALRM, "SIGALRM"},
        {SIGTERM, "SIGTERM"},
    #ifdef SIGSTKFLT
        {SIGSTKFLT, "SIGSTKFLT"},
    #endif
        {SIGCHLD, "SIGCHLD"},
        {SIGCONT, "SIGCONT"},
        {SIGSTOP, "SIGSTOP"},
        {SIGTSTP, "SIGTSTP"},
        {SIGTTIN, "SIGTTIN"},
        {SIGTTOU, "SIGTTOU"},
        {SIGURG, "SIGURG"},
        {SIGXCPU, "SIGXCPU"},
        {SIGXFSZ, "SIGXFSZ"},
        {SIGVTALRM, "SIGVTALRM"},
        {SIGPROF, "SIGPROF"},
        {SIGWINCH, "SIGWINCH"},
        {SIGIO, "SIGIO"},
    #ifdef SIGPWR
        {SIGPWR, "SIGPWR"},
    #endif
    #ifdef SIGSYS
        {SIGSYS, "SIGSYS"},
    #endif
    };

    auto it = signal_names.find(signal);
    return (it != signal_names.end()) ? it->second : "UNKNOWN";
}


#pragma endregion

volatile sig_atomic_t interrupted = 0; // For Ctrl+C 
volatile sig_atomic_t tstp        = 0;

void handle_sigint(int) { interrupted = 1; }
void handle_sigtstp(int) { tstp = 1; info::debug("debug");};

std::string message(int sig, bool core_dumped) {
    std::string red   = "\033[31m";
    std::string yellow= "\033[33m";
    std::string cyan  = "\033[36m";
    std::string reset = "\033[0m";

    // Map signals to colors
    static std::map<int, std::string> sig_color = {
        {SIGHUP, cyan}, {SIGINT, yellow}, {SIGQUIT, red}, {SIGILL, red},
        {SIGTRAP, red}, {SIGABRT, red}, {SIGBUS, red}, {SIGFPE, red},
        {SIGKILL, red}, {SIGUSR1, yellow}, {SIGSEGV, red}, {SIGUSR2, yellow},
        {SIGPIPE, cyan}, {SIGALRM, cyan}, {SIGTERM, yellow}, {SIGSTKFLT, red},
        {SIGCHLD, cyan}, {SIGCONT, cyan}, {SIGSTOP, red}, {SIGTSTP, yellow},
        {SIGTTIN, cyan}, {SIGTTOU, cyan}, {SIGURG, cyan}, {SIGXCPU, red},
        {SIGXFSZ, red}, {SIGVTALRM, cyan}, {SIGPROF, cyan}, {SIGWINCH, cyan},
        {SIGPOLL, cyan}, {SIGPWR, red}, {SIGSYS, red}
    };

    const char* sig_name = strsignal(sig); // human-readable signal name
    std::string color = sig_color.count(sig) ? sig_color[sig] : yellow;

    if(color == red) {
        std::stringstream ss;
        ss << red << "[Terminated: " << strsignal(sig);
        if(core_dumped) ss << " (core dumped)";
        ss << "]";
        return ss.str();
    } else if(color == yellow) {
        return yellow + "[" + sig_name + "]" + reset;
    } else { // cyan
        return cyan + "[Notification: " + sig_name + "]" + reset;
    }
}

int wait_foreground_job(pid_t pid, const std::string& cmd, ExecFlags flags, std::chrono::_V2::system_clock::time_point start) {
    struct sigaction in{};
    in.sa_handler = handle_sigint;
    sigemptyset(&in.sa_mask);
    in.sa_flags = 0;
    sigaction(SIGINT, &in, nullptr);
      
    struct sigaction stop{};
    stop.sa_handler = handle_sigtstp;
    sigemptyset(&stop.sa_mask);
    stop.sa_flags = 0;
    sigaction(SIGTSTP, &stop, nullptr);

    //int flags_fd = fcntl(STDIN_FILENO, F_GETFL, 0);
    //fcntl(STDIN_FILENO, F_SETFL, flags_fd | O_NONBLOCK);

    int status;

    setpgid(pid, pid);

    while (true) {
        tcsetpgrp(STDIN_FILENO, pid);
        pid_t result = waitpid(pid, &status, WUNTRACED | WCONTINUED);
    if (result == -1) break; // no more children

        if((WIFEXITED(status) || WIFSIGNALED(status)) && flags.time) {
      auto end = std::chrono::high_resolution_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      int hours = elapsed.count() / 3600000;
      int minutes = (elapsed.count() % 3600000) / 60000;
      int seconds = (elapsed.count() % 60000) / 1000;
      int milliseconds = elapsed.count() % 1000;

      std::stringstream ss;
    
      ss << cyan << "[Elapsed time: "
         << std::setw(2) << std::setfill('0') << hours << ":"
         << std::setw(2) << std::setfill('0') << minutes << ":"
         << std::setw(2) << std::setfill('0') << seconds << ":"
         << std::setw(3) << std::setfill('0') << milliseconds
         << "]\n" << reset;

      io::print(ss.str());       
        }

        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (flags.exit_code) {
                if (code == 0) io::print(green + "[Process exited with code 0]" + reset + "\n");
                else io::print(red + "[Process exited with code " + std::to_string(code) + "]\n" + reset);
            }
            //if(repeat && code != 0) execute()
            tcsetpgrp(STDIN_FILENO, getpgrp());
            JobCont::update_job(pid, JobCont::State::Completed);
            return code;
        }
        else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            io::print(message(sig, WCOREDUMP(status)) + "\n");
            tcsetpgrp(STDIN_FILENO, getpgrp());
            JobCont::update_job(pid, JobCont::State::Interrupted);
            
            return 128 + sig;
        }
        else if (WIFSTOPPED(status)) {
            JobCont::add_job(pid, cmd, JobCont::State::Stopped, flags, start);
            io::print(orange + "[Process " + cmd + " stopped]\n" + reset);
            tcsetpgrp(STDIN_FILENO, getpgrp());
            return 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
    return 0;
}



int execute(std::vector<std::string> parsed_args, std::string input, bool bg, RedirectInfo rinfo = {}, ExecFlags info = {}) {
  if(parsed_args.empty()) return 0;

  std::vector<std::string> working_args = parsed_args;
  for (size_t i = 0; i < working_args.size(); ) {
      std::string& s = working_args[i];
      bool is_opr = s == "@e" || s == "@o" || s == "@O" || s == "@t" || s == "@r" ||
                    s == ">" || s == ">>" || s == "1>" || s == "1>>" ||
                    s == "2>" || s == "2>>" ||
                    s == "<";
      if (is_opr) {
          if (i + 1 < working_args.size() &&
              (s == ">" || s == ">>" || s == "1>" || s == "1>>" ||
              s == "2>" || s == "2>>" || s == "<")) {
              working_args.erase(working_args.begin() + i, working_args.begin() + i + 2);
          } else {
              working_args.erase(working_args.begin() + i);
          }
      } else {
          ++i;
      }
  }

  if (working_args.empty()) {
    info::error("No command specified.");
    return -1;
  }

  std::string cmd = parsed_args[0];

  bool using_path = false;
  std::string home = getenv("HOME");
  std::string slashutil_path = home + "/.slash/slash-utils/" + parsed_args[0];

  // For the condition with ~, .., and /, trying to access a slash-util with these will disrupt it
  if(access(slashutil_path.c_str(), X_OK) == 0 && 
   parsed_args[0] != "~" && parsed_args[0] != "/" && parsed_args[0] != "..") {
       parsed_args[0] = slashutil_path;
       using_path = true;
}

  bool stdout_only   = io::vecContains(parsed_args, "@o");
  bool stderr_only   = io::vecContains(parsed_args, "@O");
  bool time          = io::vecContains(parsed_args, "@t");
  bool repeat        = io::vecContains(parsed_args, "@r");
  bool print_exit    = io::vecContains(parsed_args, "@e") || is_print_exit_code_enabled();

    struct ExecFlags info_to_use;
    if(sizeof(info_to_use) == 1) { // empty structs have size 1
        info_to_use.time = time;
        info_to_use.repeat = repeat;
        info_to_use.exit_code = print_exit;
    } else info_to_use = info;

  if(!parsed_args.empty()) {
    while(parsed_args.back() == "@e" || parsed_args.back() == "@o" || parsed_args.back() == "@O" || parsed_args.back() == "@r" || parsed_args.back() == "@t") {
      parsed_args.pop_back();
    }
  }

  if(!rinfo.stdout_enabled || !rinfo.stderr_enabled) {
    for (size_t i = 0; i < parsed_args.size(); i++) {
        if (parsed_args[i] == ">" || parsed_args[i] == ">>" || parsed_args[i] == "1>" || parsed_args[i] == "1>>") {
            rinfo.stdout_enabled = true;
            rinfo.stdout_append = (parsed_args[i] == ">>" || parsed_args[i] == "1>>");
            if (i + 1 < parsed_args.size()) rinfo.stdout_filepath = parsed_args[i + 1];
            parsed_args.erase(parsed_args.begin() + i, parsed_args.begin() + i + 2);
            i--;
        }
        else if (parsed_args[i] == "2>" || parsed_args[i] == "2>>") {
            rinfo.stderr_enabled = true;
            rinfo.stderr_append = (parsed_args[i] == "2>>");
            if (i + 1 < parsed_args.size()) rinfo.stderr_filepath = parsed_args[i + 1];
            parsed_args.erase(parsed_args.begin() + i, parsed_args.begin() + i + 2);
            i--;
        }
        else if (parsed_args[i] == "2>&1") rinfo.err_to_out = true;
        else if (parsed_args[i] == "1>&2") rinfo.out_to_err = true;
        else if (parsed_args[i] == "0>&1") rinfo.in_to_out = true;
        else if (parsed_args[i] == "0>&2") rinfo.in_to_err = true;
        else if (parsed_args[i] == "2>&0") rinfo.err_to_in = true;
        else if (parsed_args[i] == "1>&0") rinfo.out_to_in = true;
    }
  }

  std::string input_redfile;
  if(io::vecContains(parsed_args, "<")) {
    auto it = std::find_if(parsed_args.begin(), parsed_args.end(), [](std::string s) {
      return s == "<";
    });
    int i = std::distance(parsed_args.begin(), it);

    if(i >= parsed_args.size()) {
      info::error("Unspecified input redirection file");
      return -1;
    }

    input_redfile = parsed_args[i + 1];
    parsed_args.erase(parsed_args.begin() + i, parsed_args.begin() + i + 2);
  }

  parsed_args.erase(
        std::remove_if(parsed_args.begin(), parsed_args.end(),
                        [](std::string s){
                            return s == "2>&0" || s == "2>&1" || 
                                  s == "1>&0" || s == "1>&2" || 
                                  s == "0>&1" || s == "0>&2";
                        }),
          parsed_args.end()
      );

  if(parsed_args[0] == "var") {
    parsed_args.erase(parsed_args.begin());
    
    return var(parsed_args);
  }

  if(parsed_args[0] == "jobs") {
    parsed_args.erase(parsed_args.begin());
    return jobs(parsed_args);
  }

  if(parsed_args[0] == "help") {
    if(parsed_args.size() > 1) {
      if(parsed_args[1] == "--slash-utils") return slash_utils_help();
      else if(parsed_args[1] == "--keys") return help_keys();
      else info::error("Invalid help argument.\n");
    }
    return help();
  }

  if(parsed_args[0] == "alias") {
    parsed_args.erase(parsed_args.begin());
    return alias(parsed_args);
  }

  if (parsed_args[0] == "exit") {
        if(!JobCont::jobs.empty()) {
            std::vector<JobCont::Job> non_completed_jobs;
            for(auto& job : JobCont::jobs) {
                if(job.jobstate != JobCont::State::Completed) non_completed_jobs.push_back(job);
            }
            if(!non_completed_jobs.empty()) {
                std::string first = JobCont::jobs.size() == 1 ? "There is 1 uncompleted job." : "There are " + std::to_string(JobCont::jobs.size()) + " uncompleted jobs.";
                info::warning(first + " Are you sure you want to exit slash? (Y/N): ");
                char c;
                ssize_t bytesRead = read(STDIN_FILENO, &c, 1);
                if(bytesRead < 0) {
                    info::error("Failed to read stdin: " + std::string(strerror(errno)));
                    return -1;
                }
                io::print("\n");
                if(tolower(c) != 'y') {
                    return 0;
                }
            }
        }
         enable_canonical_mode();
        exit(0);
    }

    if (parsed_args[0] == "cd") {
        parsed_args.erase(parsed_args.begin());
        return cd(parsed_args);
    }

    if(parsed_args[0] == "~" || parsed_args[0] == ".." || parsed_args[0] == "/") {
      return cd({parsed_args[0]});
    }

    if(parsed_args[0] == "slash-greeting") return greet();

  pid_t pid = fork();
  if (pid == -1) {
    info::error(strerror(errno), errno);
    return errno;
  }

  auto start = std::chrono::high_resolution_clock::now();
  int job_index = 0;
    
  if(bg) {
    JobCont::add_job(pid, parsed_args[0], JobCont::State::Running, info_to_use, start);
    job_index = JobCont::jobs.size() - 1;
    info::debug("dsa");
  }

  if (pid == 0) {
    setpgid(0, 0);
    enable_canonical_mode();

    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    if(rinfo.stdout_enabled || rinfo.stderr_enabled) {
      int flags = O_WRONLY | O_CREAT; // Create if doesn't exist
      if(rinfo.stdout_append || rinfo.stderr_append) flags |= O_APPEND;
      else flags |= O_TRUNC;

      if(rinfo.stdout_enabled) {
        int fd = open(rinfo.stdout_filepath.c_str(), flags, 0644);
        if(fd < 0) {
          info::error("Failed to open " + rinfo.stdout_filepath + ": " + std::string(strerror(errno)));
          _exit(errno);
        }

        dup2(fd, STDOUT_FILENO);
      }

      if(rinfo.stderr_enabled) {
        int fd = open(rinfo.stderr_filepath.c_str(), flags, 0644);
        if(fd < 0) {
          info::error("Failed to open " + rinfo.stderr_filepath + ": " + std::string(strerror(errno)));
          _exit(errno);
        }

        dup2(fd, STDERR_FILENO);
      }


      if (rinfo.err_to_in)   dup2(STDIN_FILENO, STDERR_FILENO);
      if (rinfo.err_to_out)  dup2(STDOUT_FILENO, STDERR_FILENO);
      if (rinfo.out_to_in)   dup2(STDIN_FILENO, STDOUT_FILENO);
      if (rinfo.out_to_err)  dup2(STDERR_FILENO, STDOUT_FILENO);
      if (rinfo.in_to_out)   dup2(STDOUT_FILENO, STDIN_FILENO);
      if (rinfo.in_to_err)   dup2(STDERR_FILENO, STDIN_FILENO);

    }

    if(!input_redfile.empty()) {
      int fd = open(input_redfile.c_str(), O_RDONLY);
      if(fd < 0) {
        info::error("Failed to open \"" + input_redfile + "\": " + std::string(strerror(errno)));
        return errno;
      }
      if(dup2(fd, STDIN_FILENO) < 0) {
        info::error("Failed to redirect file to stdin: " + std::string(strerror(errno)));
        return errno;
      }
    }

    if(time) {
      io::print(cyan + "[Timer started]\n" + reset);
    }

    int devnu = open("/dev/null", O_RDWR);

    if(stdout_only) dup2(devnu, STDERR_FILENO);
    if(stderr_only) dup2(devnu, STDOUT_FILENO);

    std::vector<char*> argv;
    for (auto& arg : parsed_args) {
      argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); // NULL-terminate

    std::string joined;
    for (int i = 0; argv[i] != nullptr; i++) {
      if (i > 0) joined += ' ';
      joined += argv[i];
    }

    if(using_path) {
        execv(slashutil_path.c_str(), argv.data());
    } else {
        execvp(argv[0], argv.data());
    }

    // If execvp or execv fail
    if(errno == ENOENT) {
      io::print_err(cnf(parsed_args[0]));
    } else {
      info::error("Failed to execute \"" + parsed_args[0] + "\": " + std::string(strerror(errno)));
    }
    _exit(errno); // terminate child
  } else {
    signal(SIGTTOU, SIG_IGN);
    setpgid(pid, pid);

    if (!bg) {
      return wait_foreground_job(pid, cmd, info_to_use, start);
    } else {
        io::print(yellow + "[Process running in the background, pid " + std::to_string(pid) + "]\n" + reset);
                enable_raw_mode();    

        std::thread([pid, cmd]() {
                    int status;
                    if (waitpid(-pid, &status, 0) > 0) {
                        std::string msg;
                        if (WIFEXITED(status)) {
                            int code = WEXITSTATUS(status);
                            msg = orange + "[Background process " + cmd + " finished with exit code " + std::to_string(code) + "]\n" + reset;
                                            JobCont::update_job(pid, JobCont::State::Completed);
                        } else if (WIFSIGNALED(status)) {
                            int sig = WTERMSIG(status);
                            msg = orange + "[Background process " + cmd + " terminated by signal " + std::to_string(sig) + "]\n" + reset;
                                            JobCont::update_job(pid, JobCont::State::Interrupted);
                        }
                        io::print(msg);
                    }
                    }).detach();

        return 0;
    }
  }
}


int execute_pipeline(const std::string& raw_input) {
    auto commands = io::split(raw_input, "|");
    int n = commands.size();

    int prev_fd = -1; // previous pipe read end
    std::vector<pid_t> pids;

    for (int i = 0; i < n; i++) {
        std::string cmd_str = io::trim(commands[i]);
        auto args = parse_arguments(cmd_str);

        int pipe_fd[2] = {-1, -1};
        if (i < n - 1) {
            if (pipe(pipe_fd) == -1) {
                info::error("Failed to create pipe: " + std::string(strerror(errno)), errno);
                return -1;
            }
        }

        pid_t pid = fork();
        if (pid == -1) {
            info::error("Fork failed: " + std::string(strerror(errno)), errno);
            return -1;
        }

        if (pid == 0) { // child
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (pipe_fd[1] != -1) {
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }

            execute(args, cmd_str, io::vecContains(args, "&"), {});
            _exit(0);
        } else { // parent
            if (prev_fd != -1) close(prev_fd);
            if (pipe_fd[1] != -1) close(pipe_fd[1]);
            prev_fd = pipe_fd[0];
            pids.push_back(pid);
        }
    }

    // Wait for all children
    int exit_code = 0;
    for (auto pid : pids) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
    }

    return exit_code;
}

int exec(std::vector<std::string> args, std::string raw_input) {
    raw_input = io::trim(raw_input);
    if (args.empty()) return 0;

    bool bg = false;
    if (raw_input.ends_with("&")) {
        bg = true;
        args.pop_back();
    }

    // Handle multiple commands separated by ";"
    if (io::vecContains(args, ";")) {
        for (auto cmd : io::split(raw_input, ";")) {
            cmd = io::trim(cmd);
            int res = exec(parse_arguments(cmd), cmd);
        }
        return 0;
    }

    // Handle "&&"
    if (io::vecContains(args, "&&")) {
        for (auto cmd : io::split(raw_input, "&&")) {
            cmd = io::trim(cmd);
            int res = exec(parse_arguments(cmd), cmd);
            if (res != 0) break;
        }
        return 0;
    }

    // Handle "||"
    if (io::vecContains(args, "||")) {
        for (auto cmd : io::split(raw_input, "||")) {
            cmd = io::trim(cmd);
            int res = exec(parse_arguments(cmd), cmd);
            if (res == 0) break;
        }
        return 0;
    }

    if (io::vecContains(args, "|")) {
        return execute_pipeline(raw_input);
    }

    return execute(args, raw_input, bg, {});
}
