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
	std::vector<int> pipefds(2 * (n - 1)); // Two file descriptors per pipe

	// Create all necessary pipes
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

		if (pid == 0) { // Child process
			// If not the first command, read from previous pipe
			if (i != 0) {
				dup2(pipefds[2 * (i - 1)], STDIN_FILENO);
			}
			// If not the last command, write to next pipe
			if (i != n - 1) {
				dup2(pipefds[2 * i + 1], STDOUT_FILENO);
			}

			// Close all pipe fds in child
			for (int fd : pipefds) close(fd);

			// Build argv
			std::vector<char*> argv;
			for (auto& arg : commands[i]) {
				argv.push_back(const_cast<char*>(arg.c_str()));
			}
			argv.push_back(nullptr);

			// Execute
			execvp(argv[0], argv.data());

			// If execvp fails
			std::string error = "Execution failed: " + std::string(strerror(errno));
			info::error(error, errno);
			exit(errno);
		}
	}

	// Parent process: close all fds
	for (int fd : pipefds) close(fd);

	// Wait for all children
	for (int i = 0; i < n; ++i) {
		int status;
		wait(&status);
	}

	return 0;
}

int execute(std::vector<std::string> args, bool save_to_history) {
	if (args.empty()) return 0;

	std::string cmd = args[0]; // To save the original command name instead of ~/slash-utils/cmd if it's a slash utility

	bool using_path = false;
	const char* home_env = getenv("HOME");
	std::string home_dir = home_env ? home_env : "";

	// If command is a slash utility, change the path of the first argument
	std::string cmd_path = std::string(home_env) + "/.slash/slash-utils/" + args[0];
	if (access(cmd_path.c_str(), X_OK) == 0) {
		args[0] = cmd_path;
		using_path = true;
	}

	std::vector<std::string> original_cmd;
	if(using_path) {
		for(int i = 0; i < args.size(); i++) {
			if(i == 0) {
				original_cmd.push_back(cmd + " ");
				continue;
			}
			original_cmd.push_back(args[i]);
		}
	}

	if (save_to_history) {
		std::string history_path = std::string(home_env) + "/.slash/.slash_history";
		auto data = using_path ? io::join(original_cmd, " ") : io::join(args, " ");
		data = io::trim(data);

		if (io::write_to_file(history_path, data + "\n") != 0) {
			std::string error = std::string("Failed to save command to history: ") + strerror(errno);
			info::error(error, errno, history_path);
			return -1;
		}
	}

	if (args[0] == "cd") {
		args.erase(args.begin()); // Avoid executing "cd cd [args]"
		cd(args);
		return 0;
	}

	if(args[0] == "var") {
		args.erase(args.begin());
		var(args);
		return 0;
	}

	pid_t pid = fork();
	if (pid == -1) {
		info::error(strerror(errno), errno);
		return -1;
	}

	if (pid == 0) {
		std::vector<char*> argv;
		for (auto& arg : args) {
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
		std::string err = std::string("\"" + args[0] + "\"" + "Execution failed: ") + strerror(errno);
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
