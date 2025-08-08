#include "startup.h"
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "execution.h"

static termios orig;

void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void enable_raw_mode() {
	tcgetattr(STDIN_FILENO, &orig);
	atexit(disable_raw_mode);

	termios raw = orig;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_lflag &= ~(OPOST);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

///////////////////////////////////////////////////////

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
	auto startup_commands = io::read_file(slash_dir + "/.slash_startup_commands");

	if(std::holds_alternative<int>(startup_commands)) {
		info::error(strerror(errno), errno, ".slash_startup_commands");
		return;
	}

	std::vector<std::string> lines = io::split(std::get<std::string>(startup_commands), "\n");
	for(auto& command : lines) {
		command = interpret_escapes(command);
		if(command.empty()) continue;
		std::vector<std::string> tokens = io::split(command, " ");
		execute(tokens, io::join(tokens, " "), false, false, false);
	}
}

