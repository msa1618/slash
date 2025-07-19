#include "../command.h"
#include "../abstractions/iofuncs.h"
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
									"usage: read <filepath>\n");
				return 0;
			}

			std::string path;
			for(auto& arg : args) {
				if(!arg.starts_with("-")) {  // Will make the condition better later
					path = arg;
				}
			}

			int fd = open(path.c_str(), O_RDONLY);
			if(fd == -1) {
				perror("Failed to open file");
			}

			std::vector<std::string> data;

			char buffer[1024];
			ssize_t bytesRead;

			while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
				buffer[bytesRead] = '\0';
				data.emplace_back(buffer);
			}

			for(auto& arg : args) {
				if(arg == "-r" || arg == "--reverse") {
					std::stringstream ss;
					for (auto& chunk : data) {
						ss << chunk;
					}
					std::string full = ss.str();
					std::reverse(full.begin(), full.end());
					data = io::split(full, ' ');
				}

				if(arg == "-rl" || arg == "--reverse-lines") {
					std::stringstream ss;
					for(auto& letter : data) {
						ss << letter;
					}
					auto vec = io::split(ss.str(), ' ');
					std::reverse(vec.begin(), vec.end());
					data = io::split(io::join(vec, "\n"), ' ');
				}
			}

			if (bytesRead == -1) {
				perror("read failed");
			}

			std::vector<std::string> lines = io::split(io::join(data, ""), '\n');

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

			close(fd);
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
