#include "../command.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/definitions.h"
#include <iomanip>
#include <iostream>

class Ansi : public Command {
	public:
		Ansi() : Command("ansi", "Displays all ANSI color codes and formatting options", "") {};

	int exec(std::vector<std::string> args) {
		for(int i = 30; i < 37; i++) {
			std::cout << "\x1b[" << i << "m";
			std::cout << std::setw(5) << std::setfill(' ') << i;
			std::cout << reset;
		}

		std::cout << "\n";

		for(int i = 40; i < 47; i++) {
			std::cout << "\x1b[" << i << "m";
			std::cout << std::setw(5) << std::setfill(' ') << i;
			std::cout << reset;
		}

		std::cout << "\n\n256 color palette\n-----------------\n";
		for (int i = 0; i < 256; ++i) {
			std::cout << "\x1b[48;5;" << i << "m";
			std::cout << std::setw(5) << std::setfill(' ') << i;
			std::cout << reset;

			if ((i + 1) % 16 == 0) std::cout << "\n";
		}

		std::cout << "\n\nANSI formatting options\n-----------------------\n";
		std::cout << reset << "[0] This is normal text.\033[0m\n";
		std::cout << bold << "[1] This is bold text.\033[0m\n";
		std::cout <<  "\033[2m[2] This is dim text.\033[0m\n";
		std::cout << "\033[3m[3] This is italic text.\033[0m\n";
		std::cout << "\033[4m[4] This is underlined text.\033[0m\n";
		std::cout << "\033[5m[5] This is blinking text.\033[0m\n";
		std::cout << "\033[9m[9] This is strikethrough text.\033[0m\n";
		std::cout << "\033[1mNote:\033[0m Some terminals might not support some formatting options.\n";


		return 0;
	}



};

int main(int argc, char* argv[]) {
	Ansi ansi;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	ansi.exec(args);
	return 0;
}
