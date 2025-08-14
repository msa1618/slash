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
volatile sig_atomic_t kill_req    = 0;

void handle_sigint(int) {
	interrupted = 1;
}

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
				if(core_dumped) ss << "(core dumped)";
				ss << "]";
				return ss.str();
    } else if(color == yellow) {
        return yellow + "[" + sig_name + "]" + reset;
    } else { // cyan
        return cyan + "[Notification: " + sig_name + "]" + reset;
    }
}

int pipe_execute(std::vector<std::vector<std::string>> commands) {
    int n = commands.size();
    if (n == 0) return 0;

    std::vector<int> pipefds(2 * (n - 1)); // Two fds per pipe
    std::vector<pid_t> pids;

    // Create pipes
    for (int i = 0; i < n - 1; ++i) {
        if (pipe(&pipefds[2 * i]) < 0) {
						std::string error = std::string("Failed to create pipe: ") + strerror(errno);
            info::error("Failed to create pipe", errno);
            return errno;
        }
    }

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            info::error("Fork failed", errno);
            return -1;
        }

        if (pid == 0) { // Child
            // If not first command, set stdin to read end of previous pipe
            if (i != 0) {
                dup2(pipefds[2 * (i - 1)], STDIN_FILENO);
            }
            // If not last command, set stdout to write end of current pipe
            if (i != n - 1) {
                dup2(pipefds[2 * i + 1], STDOUT_FILENO);
            }

            // Close all pipe fds in child (they are duplicated if needed)
            for (int fd : pipefds) close(fd);

            // Build argv
            std::vector<char*> argv;
            for (auto& arg : commands[i]) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);

            // Execute command
            execvp(argv[0], argv.data());

            // If execvp fails
            std::string error = "Failed to execute \"" + std::string(argv[0]) + "\": " + strerror(errno);
            info::error(error, errno);
            exit(errno);
        }

        pids.push_back(pid);
    }

    // Parent closes all pipe fds
    for (int fd : pipefds) close(fd);

    // Wait for all children
    int exit_status = 0;
    for (pid_t pid : pids) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            info::error("waitpid failed", errno);
            return -1;
        }
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (code != 0) exit_status = code; // capture last non-zero exit code
        }
    }

    return exit_status;
	}

int execute(std::vector<std::string> parsed_args, std::string input, bool save_to_history, bool bg, RedirectInfo rinfo) {
	if (parsed_args.empty()) return 0;

	std::string cmd = parsed_args[0]; // To save the original command name instead of ~/slash-utils/cmd if it's a slash utility

	bool using_path = false;
	const char* home_env = getenv("HOME");
	std::string home_dir = home_env ? home_env : "";

	// If command is a slash utility, change the path of the first argument
	std::string cmd_path = std::string(home_env) + "/.slash/slash-utils/" + parsed_args[0];
	if (access(cmd_path.c_str(), X_OK) == 0) {
		parsed_args[0] = cmd_path;
		using_path = true;
	}

	bool stdout_only   = io::vecContains(parsed_args, "@o");
	bool stderr_only   = io::vecContains(parsed_args, "@O");
	bool time          = io::vecContains(parsed_args, "@t");
	bool repeat        = io::vecContains(parsed_args, "@r");
	bool print_exit    = io::vecContains(parsed_args, "@e") || is_print_exit_code_enabled();

	if(!parsed_args.empty()) {
		while(parsed_args.back() == "@e" || parsed_args.back() == "@o" || parsed_args.back() == "@O" || parsed_args.back() == "@r" || parsed_args.back() == "@t") {
			parsed_args.pop_back();
		}
	}

	std::vector<std::string> original_cmd = io::split(input, " ");
	original_cmd[0] = cmd;

	if (save_to_history) {
		std::string history_path = std::string(home_env) + "/.slash/.slash_history";
		auto data = using_path ? io::join(original_cmd, " ") : input;
		data = io::trim(data);

		if (io::write_to_file(history_path, data + "\n") != 0) {
			std::string error = std::string("Failed to save command to history: ") + strerror(errno);
			info::error(error, errno, history_path);
			return -1;
		}
	}

	if (parsed_args[0] == "cd") {
		// Avoid executing "cd cd [parsed_args]"
		cd(parsed_args);
		return 0;
	}

	if(parsed_args[0] == "var") {
		parsed_args.erase(parsed_args.begin());
		var(parsed_args);
		return 0;
	}

	if(parsed_args[0] == "alias") {
		parsed_args.erase(parsed_args.begin());
		return alias(parsed_args);
	}

	pid_t pid = fork();
	if (pid == -1) {
		info::error(strerror(errno), errno);
		return -1;
	}

	auto start = std::chrono::high_resolution_clock::now();

	if (pid == 0) {
		setpgid(0, 0);
		enable_canonical_mode();
		signal(SIGINT, SIG_DFL);

		if(rinfo.enabled) {
			int flags = O_WRONLY | O_CREAT; // Create if doesn't exist
			if(rinfo.append) flags |= O_APPEND;
			else flags |= O_TRUNC;

			int fd = open(rinfo.filepath.c_str(), flags, 0644);
			if(fd < 0) {
				info::error(std::string("Failed to open file: ") + strerror(errno));
				return errno;
			}

			int stdfd = rinfo.stdout_ ? STDOUT_FILENO : STDERR_FILENO;
			dup2(fd, stdfd);
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
			execv(argv[0], argv.data());
		} else {
			execvp(argv[0], argv.data());
		}

		// If execvp or execv fail
		std::string err = std::string("\"" + parsed_args[0] + "\"" + "Failed to execute \"" + parsed_args[0] + "\": ") + strerror(errno);
		info::error(err, errno);
		exit(errno); // terminate child
	} else {
		struct sigaction sa{};
		sa.sa_handler = handle_sigint;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGINT, &sa, nullptr);

		signal(SIGTTOU, SIG_IGN);
		setpgid(pid, pid);

		if (!bg) {
				int status;
				while (true) {
						tcsetpgrp(STDIN_FILENO, pid); 
						pid_t result = waitpid(pid, &status, WNOHANG);

						if (result == pid) {
								// Child exited
								tcsetpgrp(STDIN_FILENO, getpgrp()); // return control to shell

								if(time) {
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
								signal(SIGTTOU, SIG_DFL);
								if (WIFEXITED(status)) {
										int errcode = WEXITSTATUS(status);
										if(print_exit) {
											if(errcode == 0) io::print(green + "[Process exited with exit code 0]" + reset + "\n");
											else io::print(red + "[Process exited with exit code " + std::to_string(errcode) + "]\n" + reset);
										}
										if(errcode != 0 && repeat) {
											return execute(parsed_args, input, save_to_history, bg, rinfo);
										}
										return errcode;
								} else if (WIFSIGNALED(status)) {
										int sig = WTERMSIG(status);
										bool dumped = WCOREDUMP(status);
										
										io::print(message(sig, dumped) + "\n");
										return 128 + sig;
								}
								break;
						}
						if (interrupted) {
								kill(pid, SIGINT);
								break;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Small sleep to avoid busy loop
				}
		} else {
				setpgid(pid, pid);
				io::print(yellow + "[Process running in the background, pid " + std::to_string(pid) + "]\n" + reset);
				return 0;
		}
	}
}
