#include <unistd.h>
#include <string>
#include <vector>

#include "../abstractions/iofuncs.h"
#include "../command.h"

class Del : public Command {
	public:
	Del() : Command("del", "Deletes files, directories, and links permanently", "") {}

	int exec(std::vector<std::string> args) {
		if(args.empty()) {
			io::print("del: deletes one or more files, directories (coming soon), and links permanently\n"
								"usage: del <file-1> <file-2> ...\n"
								"flags:\n"
								"-p: prompt before every deletion\n"
								"-i: delete immediately, no prompt\n");
			return 0;
		}

		bool prompt = false;

		for(auto& arg : args) {
			if(arg == "-p") prompt = true;
			else if(arg == "-i") prompt = false;
		}

		for(auto& arg : args) {
			if(arg.starts_with('-')) continue;

			if(prompt) {
				io::print("Are you sure you want to delete ");
				io::print(arg.c_str());
				io::print("? (y/n)");

				char buffer[3];
				ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
				if(bytesRead < 0) {
					perror("del");
					return -1;
				}
				buffer[bytesRead] = '\0';

				std::string answer = std::string(buffer);

				if (!answer.empty() && answer.back() == '\n')
					answer.pop_back();

				for(auto& ch : answer) { ch = tolower(ch); }
				if(answer == "y" || answer == "yes" || answer == "yeah" || answer == "ye") {
					if(unlink(arg.c_str()) == -1) {
						perror("del");
					}
				} else continue;
			} else {
				if(unlink(arg.c_str()) == -1) {
					perror("del");
				}
			}
		}

	}
};

int main(int argc, char* argv[]) {
	Del del;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	del.exec(args);
	return 0;
}
