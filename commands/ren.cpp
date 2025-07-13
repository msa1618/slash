#include "../abstractions/iofuncs.h"
#include "../command.h"
#include <unistd.h>

class Ren : public Command {
	public:
		Ren() : Command("ren", "Renames files.", "") {}

		int exec(std::vector<std::string> args) {
			if(args.empty()) {
				io::print("ren: renames files\n"
									"usage: ren <old_name> <new_name>");
				return 0;
			}

			const char* first_path = args[0].c_str();
			const char* sec_path = args[1].c_str();

			if(rename(first_path, sec_path) != 0) {
				perror("Failed to rename file");
				return 1;
			}
		}
};

int main(int argc, char* argv[]) {
	Ren ren;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	ren.exec(args);
	return 0;
}
