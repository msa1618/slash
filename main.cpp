#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <sstream>
#include <functional>
#include "abstractions/definitions.h"
#include "abstractions/iofuncs.h"

int get_terminal_width() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_col;
}

void print_right(std::string text) {
	int width = get_terminal_width();

	// std::max makes sure the column number is not less than one
	int column = std::max(1, width - static_cast<int>(text.size() + 1));

	io::print("\x1b[");
	io::print(std::to_string(column).c_str());
	io::print("G");
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
		// execvp() only returns if it fails, thats why no conditional
		perror(argv[0]);
		return -1;
	}
	else if(pid > 0) {
		int status;
		waitpid(pid, &status, 0);
		WEXITSTATUS(status);
	}

}

int main() {
	std::string command;
	std::unordered_map<std::string, std::function<int(std::vector<const char*>)>> commands;

	int fd = open(".slash_startup_commands", O_RDONLY);
	if(fd == -1) {
		perror("slash startup");
		close(fd);
		return -1;
	}

	char buffer[1024];
	ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
	if(bytesRead < 0) {
		perror("slash startup");
		close(fd);
		return -1;
	}

	buffer[bytesRead] = '\0';

	std::vector<std::string> cmds = io::split(std::string(buffer), '\n');
	for(auto& cmd : cmds) {
		std::vector<std::string> args = io::split(cmd, ' ');
		run_external_cmd(args);
	}

	close(fd);

	while(true) {
		char *cwd = getcwd(NULL, 0);
		if (!cwd) {
			perror("getcwd failed");
			return 1;
		}


		io::print("\x1b[1m\x1b[38;5;220m");
		io::print(cwd);
		io::print("\x1b[0m\n");
		print_right("test");
		io::print("\x1b[38;5;237m");
		io::print("> ");
		io::print("\x1b[0m");

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
