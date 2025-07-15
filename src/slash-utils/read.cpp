#include "../command.h"
#include "../abstractions/iofuncs.h"
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sstream>

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

			io::print(io::join(data, ""));
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
