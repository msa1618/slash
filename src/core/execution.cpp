#include "execution.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstring>
#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../builtin-cmds/cd.h"
#include "../builtin-cmds/var.h"
#include "../builtin-cmds/alias.h"
#include "startup.h"
#include <nlohmann/json.hpp>

#pragma region helpers

bool print_exit_code() {
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
		bool value = json["printExitCodeWhenProgramExits"];
		return value;
	} else {
		info::error("Missing \"printExitCodeWhenProgramExits\" property in settings");
		return false;
	}

	return false;
}

#pragma endregion

int pipe_execute(std::vector<std::vector<std::string>> commands) {
    int n = commands.size();
    if (n == 0) return 0;

    std::vector<int> pipefds(2 * (n - 1)); // Two fds per pipe
    std::vector<pid_t> pids;

    // Create pipes
    for (int i = 0; i < n - 1; ++i) {
        if (pipe(&pipefds[2 * i]) < 0) {
            info::error("Failed to create pipe", errno);
            return -1;
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

int execute(std::vector<std::string> parsed_args, std::string input, bool save_to_history) {
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

	if (pid == 0) {
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
		int status;
		waitpid(pid, &status, 0);
		if(WIFEXITED(status)) {
			int errcode = WEXITSTATUS(status);
			if(print_exit_code()) {
				if(errcode == 0) {
					io::print(green);
					io::print("[Exited with exit code 0]\n");
					io::print(reset);
				} else {
					io::print(red);
					io::print("[Exited with exit code " + std::to_string(errcode) + "]\n");
					io::print(reset);
				}
			}
			return errcode;
		}
	}
}
