#include "../command.h"

class Clear : public Command {
	public:
		Clear() : Command("clear", "Clears the entire screen."
															 "Flags:"
															 "-kcp | --keep-cursor-position: Clears the screen but cursor position stays the same."
															 " -s  |      --skip-clear     : Do not clear scrollback buffer when clearing. If not understood, try to scroll back up with this flag and without.", "") {}

		int exec(std::vector<std::string> args) {
			for(auto& arg : args) {
				if(arg == "-kcp" || arg == "--keep-cursor-position") {
					io::print("\x1b[2J");
					return 0;
				}
				if(arg == "-s" || arg == "--skip-clear") {
					io::print("\x1b[2J\x1b[H");
					return 0;
				}
			}

			io::print("\x1b[2J\x1b[3J\x1b[H");
			return 0;
		}
};

int main(int argc, char* argv[]) {
	Clear clear;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	clear.exec(args);
	return 0;
}