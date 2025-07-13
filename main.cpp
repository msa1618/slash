#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <cstring>
#include "abstractions/definitions.h"
#include "abstractions/iofuncs.h"

#pragma region helpers

int get_terminal_width() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_col;
}

int run_external_cmd(std::vector<std::string> args) {
	if(args.empty()) return 0;

	const char* home = getenv("HOME");
	std::string util_path = std::string(home) + "/slash-utils/" + args[0];

	if(access(util_path.c_str(), X_OK) == 0) {
		args[0] = util_path;
	}

	pid_t pid = fork();
	if(pid == 0) {
		std::vector<char*> argv;
		for(auto& arg : args) {
			argv.push_back(const_cast<char*>(arg.c_str()));
		}
		argv.push_back(nullptr);

		execvp(argv[0], argv.data());
		// execvp() only returns if it fails, that's why no conditional
		perror(argv[0]);
		return -1;
	}
	else if(pid > 0) {
		int status;
		waitpid(pid, &status, 0);
		WEXITSTATUS(status);
	}
}

std::string interpret_escapes(const std::string& input) {
	std::string result;
	for (size_t i = 0; i < input.size(); ++i) {
		if (input[i] == '\\' && i + 1 < input.size()) {
			char next = input[i + 1];
			if (next == 'n') {
				result += '\n';
				++i;
			} else if (next == 'x' && i + 3 < input.size()) {
				std::string hex_str = input.substr(i + 2, 2);
				char ch = (char)std::stoi(hex_str, nullptr, 16);
				result += ch;
				i += 3;
			} else {
				result += next;
				++i;
			}
		} else {
			result += input[i];
		}
	}
	return result;
}

void execute_startup_commands() {
	int fd = open(".slash_startup_commands", O_RDONLY);
	if(fd == -1) {
		perror("slash startup");
		close(fd);
		exit(-1);
	}

	char buffer[1024];
	ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
	if(bytesRead < 0) {
		perror("slash startup");
		close(fd);
		exit(-1);
	}

	buffer[bytesRead] = '\0';

	std::vector<std::string> cmds = io::split(std::string(buffer), '\n');
	for(auto& cmd : cmds) {
		std::string processed_cmd = interpret_escapes(cmd);
		std::vector<std::string> args = io::split(processed_cmd, ' ');
		run_external_cmd(args);
	}

	close(fd);
}

#pragma endregion

int main() {
	execute_startup_commands();

	while(true) {
		char *cwd = getcwd(NULL, 0);
		if (!cwd) {
			perror("getcwd failed");
			return 1;
		}

		io::print("\x1b[1m\x1b[38;5;220m");
		io::print(cwd);
		io::print(reset);
		io::print(gray);
		io::print("> ");
		io::print(reset);

		char buffer[256];
		ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

		if(bytesRead > 0) {
			buffer[bytesRead] = '\0';

			std::string input(buffer);

			if (!input.empty() && input.back() == '\n') {
				input.pop_back();
			}

			std::istringstream iss(input);
			std::vector<std::string> args;
			std::string arg;
			while (iss >> arg) {
				args.push_back(arg);
			}

			if(args.empty()) {
				continue;
			}
			if(args[0] == "cd") {
				if(chdir(args[1].c_str()) != 0) {
					perror("Failed to change directory");
				}
			} else if(args[0] == "exit") {
				io::print("Thank you for choosing Slash! ;)\n");
				return 0;
			} else {
				run_external_cmd(args);
			}

		}
	}
}
