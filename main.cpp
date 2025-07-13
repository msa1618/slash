#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <cstring>
#include "abstractions/definitions.h"
#include "abstractions/iofuncs.h"

termios orig;

#pragma region termios_funcs

void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }

void enable_raw_mode() {
	tcgetattr(STDIN_FILENO, &orig);
	atexit(disable_raw_mode);

	termios raw = orig;
	raw.c_lflag &= ~(ECHO | ICANON | ISIG);
	raw.c_lflag &= ~ICRNL;
	raw.c_lflag &= ~(OPOST);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

#pragma endregion
#pragma region helpers

int get_terminal_width() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_col;
}

int run_external_cmd(std::vector<std::string> args, bool save_to_history) {
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

	if(save_to_history) {
		io::write_to_file(".slash_history", io::join(args, " "));
		io::write_to_file(".slash_history", "\n");
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
		run_external_cmd(args, false);
	}

	close(fd);
}

std::string read_input() {
	std::string buffer;
	int char_pos = 0;
	int command_num = 0;
	char c;

	bool all_text_selected = false;

	while(true) {
		if(read(STDIN, &c, 1) != 1) continue; // Skip everything else and try again if no input was read

		if(c == '\r' || c == '\n') {
			io::print("\n");
			break;
		}

		if((c == 127 || c == 8) && !buffer.empty()) { // Backspace
			buffer.erase(buffer.begin() + char_pos - 1);
			io::print("\b \b");
			char_pos--;
		}

		if(c == 1) { all_text_selected = true; } // Ctrl + A
		if(c == 27) {
			char seq[2];
			if(read(STDIN, &seq[0], 1) != 1) continue;
			if(read(STDIN, &seq[1], 1) != 1) continue;

			if(seq[0] == '[') {
				switch(seq[1]) {
					case 'A': {  // Up
						std::string content = io::read_file(".slash_history");
						if(content.empty()) { io::print("\a"); break; }

						auto lines = io::split(content, '\n');
						if (command_num < lines.size()) {
							buffer = lines[lines.size() - 1 - command_num];
							command_num++;
						} else {
							command_num = 0;
						}
						buffer.clear();
						io::print("\r\x1b[K");
						io::print(gray);
						io::print("> ");
						io::print(reset);
						io::print(lines[command_num]);
						buffer = lines[command_num];
						command_num++;
					};
					case 'B': { // Down
						break; // later
					};
					case 'C': { // Right
						if(char_pos < buffer.size()) {
							io::print("\x1b[C");
							char_pos++;
						}
						break;
					}
					case 'D': { // Left
						if(char_pos > 0) {
							io::print("\x1b[D");
							char_pos--;
						}
						break;
					}
				}
			}
		}

		if(isprint(c)) {
			buffer.insert(buffer.begin() + char_pos, c); // Insert at current position
			char_pos++;

			// Save cursor position
			io::print("\x1b[s");
			io::print(buffer.substr(char_pos - 1));
			io::print(" "); // Overwrite old character if buffer was longer before

			// Restore cursor to original insert point
			io::print("\x1b[u\x1b[C");
		}
	}

	return buffer;
}

#pragma endregion

int main() {
	execute_startup_commands();
	enable_raw_mode();

	while(true) {
		char *cwd = getcwd(nullptr, 0);
		if (!cwd) {
			perror("getcwd failed");
			return 1;
		}

		io::print("\x1b[1m\x1b[38;5;220m");
		io::print(cwd);
		io::print(reset);
		io::print("\n");
		io::print(gray);
		io::print("> ");
		io::print(reset);
		std::string input = read_input();

			std::vector<std::string> args = io::split(input, ' '); // PLEASE DON'T KILL ME FOR THIS I PROMISE I WILL CHANGE IT

			if(args.empty()) continue;

			if(args[0] == "cd") {
				if(chdir(args[1].c_str()) != 0) {
					perror("Failed to change directory");
				}
			} else if(args[0] == "exit") {
				io::print("Thank you for choosing Slash! ;)\n");
				exit(0);
			} else {
				run_external_cmd(args, true);
			}
	}
}
