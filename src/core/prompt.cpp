#include "prompt.h"

#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <iostream>  // For isprint
#include <cstring>   // For strerror
#include "../abstractions/definitions.h"
#include "../git/git.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "execution.h"

#pragma region helpers

bool is_ssh_server() {
	return getenv("SSH_CONNECTION") != nullptr || getenv("SSH_CLIENT") != nullptr || getenv("SSH_TTY") != nullptr;
}

std::string get_battery_interface_path() {
	std::vector <std::string> files;
	DIR *dir = opendir("/sys/class/power_supply/");
	if (!dir) return "";

	struct dirent *entry;
	while ((entry = readdir(dir)) != nullptr) {
		std::string name = entry->d_name;
		if (name == "AC" || name == "ACAD") continue;
		else if (name.starts_with("BAT") || name.starts_with("CMB")) {
			files.push_back(name);
		}
	}
	closedir(dir);

	if (files.empty()) return "";
	return files[0];
}

#pragma endregion

std::variant<std::string, int> read_input(int& history_index) {
	std::string buffer;
	int char_pos = 0;
	history_index = 0;
	char c;

	while(true) {
		if(read(STDIN_FILENO, &c, 1) != 1) continue;

		if(c == '\n') {
			io::print("\n");
			break;
		}

		if (c == 127 || c == 8) {
			if (char_pos > 0) {
				char_pos--;
				buffer.erase(char_pos, 1);

				// Move cursor one left
				write(STDOUT_FILENO, "\b", 1);

				// Reprint the rest of the buffer from cursor
				std::string tail = buffer.substr(char_pos);
				io::print(tail);

				// Add a space at the end to erase leftover char
				io::print(" ");

				// Move cursor back to correct position
				for (size_t i = 0; i < tail.length() + 1; ++i) {
					write(STDOUT_FILENO, "\b", 1);
				}
			}
			continue;
		}

		if(c == 27) { // Escape sequence
			char seq[2];
			if(read(STDIN_FILENO, &seq[0], 1) != 1) continue;
			if(read(STDIN_FILENO, &seq[1], 1) != 1) continue;

			switch(seq[1]) {
				case 'A': { // Up arrow
					const char *home = getenv("HOME");
					auto history = io::read_file(std::string(home) + "/.slash/.slash_history");
					if (std::holds_alternative<int>(history)) {
						std::string err = std::string("Failed to fetch history: ") + strerror(errno);
						info::error(err, errno, "~/.slash/.slash_history");
						return -1;
					}
					std::vector<std::string> commands = io::split(std::get<std::string>(history), "\n");
					if (commands.empty()) break;

					if (history_index < commands.size())
						history_index++;

					std::string command = commands[commands.size() - history_index];
					io::print("\x1b[2K\r"); // Clear whole line and move cursor to beginning of the current line
					io::print(gray + "> ");
					io::print(reset);
					io::print(command);
					char_pos = (int) command.length();
					buffer = command;
					break;
				}

				case 'B': { // Down arrow
					const char *home = getenv("HOME");
					auto history = io::read_file(std::string(home) + "/.slash/.slash_history");
					if (std::holds_alternative<int>(history)) {
						std::string err = std::string("Failed to fetch history: ") + strerror(errno);
						info::error(err, errno, "~/.slash/.slash_history");
						return -1;
					}
					std::vector<std::string> commands = io::split(std::get<std::string>(history), "\n");
					if (commands.empty()) break;

					if (history_index < commands.size())
						history_index--;

					std::string command = commands[commands.size() - history_index];
					io::print("\x1b[2K\r"); // Clear whole line and move cursor to beginning of the current line
					io::print(gray + "> ");
					io::print(reset);
					io::print(command);
					char_pos = (int) command.length();
					buffer = command;
					break;
				}
				case 'C': { // Right arrow
					if(char_pos < (int)buffer.size()) {
						io::print("\x1b[C");
						char_pos++;
					}
					break;
				}

				case 'D': { // Left arrow
					if(char_pos > 0) {
						io::print("\x1b[D");
						char_pos--;
					}
					break;
				}
			}
			continue;
		}

		if(isprint(static_cast<unsigned char>(c))) {
			buffer.insert(buffer.begin() + char_pos, c);
			io::print(buffer.substr(char_pos));
			char_pos++;
			// Move cursor back after printing tail
			for(size_t i = 1; i < buffer.size() - char_pos + 1; ++i) {
				io::print("\b");
			}
		}
	}

	return buffer;
}

std::string print_prompt() {
	char buffer[512];
	const char* cwd = getcwd(buffer, sizeof(buffer));

	GitRepo repo(cwd);

	bool ssh = is_ssh_server();
	std::string branch = repo.get_branch_name();

	std::string battery_path = get_battery_interface_path();
	std::string bc_path = battery_path.empty() ? "" : battery_path + "/capacity";
	auto btr_capacity = battery_path.empty() ? std::variant<std::string, int>{-1} : io::read_file(bc_path);

	std::string bs_path = battery_path.empty() ? "" : battery_path + "/status";
	auto btr_status = battery_path.empty() ? std::variant<std::string, int>{-1} : io::read_file(bs_path);

	std::stringstream prompt;
	prompt << slash_color << cwd << reset << " ";
	prompt << orange << branch << reset << " ";

	if(std::holds_alternative<std::string>(btr_capacity)) {
		int btr = std::stoi(std::get<std::string>(btr_capacity));
		std::string battery_color = green;
		if(btr < 50 && btr > 20) battery_color = yellow;
		if(btr <= 20) battery_color = red;

		prompt << battery_color << btr << "% " << reset;
	}

	if(ssh) {
		prompt << green << "ssh" << reset;
	}

	io::print(prompt.str());
	io::print("\n");
	io::print(gray);
	io::print("> ");
	io::print(reset);

	int history_index = 0;
	auto input = read_input(history_index);
	if(std::holds_alternative<int>(input)) {
		info::error(strerror(errno), errno);
		return "";
	}

	std::string input_str = std::get<std::string>(input);
	return input_str;
}
