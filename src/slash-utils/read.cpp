#include "../command.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "syntax_highlighting/cpp.h"
#include "syntax_highlighting/python.h"
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sstream>

std::string tab_to_spaces(std::string content, int indent) {
	std::string spaces(indent, ' ');
	size_t pos = 0;
	while ((pos = content.find('\t', pos)) != std::string::npos) {
		content.replace(pos, 1, spaces);
		pos += spaces.length();
	}
	return content;
}

class Read : public Command {
	public:
		Read() : Command("read", "Prints out a file's content"
														 "Flags:"
														 "-r  | --reverse:       Print fully reversed"
														 "-rl | --reverse-lines: Reverse the lines, not the text in the lines", "") {};

		int exec(std::vector<std::string> args) {
			if(args.empty()) {
				io::print("read: Print a file's content\n"
									"usage: read <filepath>\n"
									"flags:\n"
									"-r  | --reverse:       Print fully reversed\n"
									"-rl | --reverse-lines: Reverse the lines only, not the text inside.\n"
									"-h  | --hidden:        Show non-printable characters.\n");
				return 0;
			}

			std::vector<std::string> validArgs = {
				"-r", "-rl", "-h",
				"--reverse", "--reverse-lines", "--hidden"
			};

			bool show_hidden_chars = false;
			bool reverse_lines     = false;
			bool reverse_text      = false;

			int indent             = 2;

			std::string path;
			for(auto& arg : args) {
				if(!io::vecContains(validArgs, arg) && arg.starts_with("-")) {
					info::error("Invalid argument \"" + arg + "\".\n");
					return EINVAL;
				}

				if(!arg.starts_with("-")) {
					path = arg;
				}

				if(arg == "-h" || arg == "--hidden") show_hidden_chars = true;
				if(arg == "-r" || arg == "--reverse") reverse_text = true;
				if(arg == "-rl" || arg == "--reverse-lines") reverse_lines = true;
			}

			auto rf = io::read_file(path);
			if(!std::holds_alternative<std::string>(rf)) {
				std::string error = std::string("Failed to read file: ") + strerror(errno);
				info::error(error, errno);
				return errno;
			}

			std::string content = std::get<std::string>(rf);
			std::vector<std::string> lines = io::split(content, "\n");

			for(auto& l : lines) {
				if (show_hidden_chars) {
					std::string space = cyan + "·" + reset;
					std::string enter = gray + "↲" + reset;
					std::string tab = gray + "├" + std::string(indent - 2, '─') + "┤" + reset;
					std::vector<std::string> control_pics = {
						"␀", "␁", "␂", "␃", "␄", "␅", "␆", "␇",
						"␈", "␉", "␊", "␋", "␌", "␍", "␎", "␏",
						"␐", "␑", "␒", "␓", "␔", "␕", "␖", "␗",
						"␘", "␙", "␚", "␛", "␜", "␝", "␞", "␟"
					};

					std::string line;
					for (auto &c: l) {
						int code = static_cast<int>(c);
						if (c == '\n') {
							line += control_pics[code] + enter;
							continue;
						}
						if (c == '\t') {
							line += tab;
							continue;
						}
						if (c == ' ') {
							line += space;
							continue;
						}
						if (code < 32) {
							line += control_pics[code];
							continue;
						}
						line += c;
					}
					l = line + enter;
				}
			}

			for(auto& l : lines) {
				// Highlighting isn't implemented yet when there are hidden characters, otherwise you'll see terminal gore
				if(path.ends_with(".cpp") && !show_hidden_chars) l = cpp_sh(l);
				if(path.ends_with(".py") && !show_hidden_chars) l = python_sh(l);
			}

			int line_width = 0;
			int longest_num_length = std::to_string(lines.size() - 1).length(); // Line no of the last element

			line_width = std::to_string(lines.size()).length();
			if(line_width % 2 == 0) line_width += 3;
			else line_width += 2;

			for(int i = 0; i < lines.size(); i++) {
				lines[i] = tab_to_spaces(lines[i], 2);
				std::string line_no =  std::to_string(i + 1);
				line_no.resize(longest_num_length, ' ');

				io::print(std::string((line_width - 1) / 2, ' ') + " ");
				io::print(gray + line_no + reset);
				io::print(std::string((line_width - 1) / 2, ' ') +  gray + "│ " + reset);
				io::print(lines[i] + "\n");
			}

			io::print("\n");
		}
};

int main(int argc, char* argv[]) {
	Read read;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	read.exec(args);
	return 0;
}
