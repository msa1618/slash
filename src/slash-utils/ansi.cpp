#include "../command.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../abstractions/definitions.h"
#include <iomanip>
#include <iostream>
#include <regex>

#pragma region helper

void print_default() {
	for(int i = 30; i < 37; i++) {
		std::cout << "\x1b[" << i << "m";
		std::cout << std::setw(5) << std::setfill(' ') << i;
		std::cout << reset;
	}

	std::cout << "          ";

	for(int i = 90; i < 97; i++) {
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

	std::cout << "          ";

	for(int i = 100; i < 107; i++) {
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
	std::cout << "\033[6m[6] This is rapidly blinking text (very rare)\033[0m\n";
	std::cout << "\033[7m[7] This is inverted text\033[0m\n";
	std::cout << "[8] This is for invisible text, not made invisible for visual reasons\033[0m\n";
	std::cout << "\033[9m[9] This is strikethrough text.\033[0m\n";
	std::cout << "\033[1mNote:\033[0m Some terminals might not support some formatting options.\n";
}

/////////////

void print_foreground() {
	for(int i = 30; i < 37; i++) {
		std::cout << "\x1b[" << i << "m";
		std::cout << std::setw(5) << std::setfill(' ') << i;
		std::cout << reset;
	}

	std::cout << "          ";

	for(int i = 90; i < 97; i++) {
		std::cout << "\x1b[" << i << "m";
		std::cout << std::setw(5) << std::setfill(' ') << i;
		std::cout << reset;
	}

	std::cout << "\n\n256 color palette\n-----------------\n";
	for (int i = 0; i < 256; ++i) {
		std::cout << "\x1b[38;5;" << i << "m";
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
	std::cout << "\033[6m[6] This is rapidly blinking text (very rare)\033[0m\n";
	std::cout << "\033[7m[7] This is inverted text\033[0m\n";
	std::cout << "[8] This is for invisible text, not made invisible for visual reasons\033[0m\n";
	std::cout << "\033[9m[9] This is strikethrough text.\033[0m\n";
	std::cout << "\033[1mNote:\033[0m Some terminals might not support some formatting options.\n";
}
//////////////

void print_non_color() {
	io::print("ANSI codes can be used to manipulate the cursor and lines, clearing the screen, \n"
						"enabling and disabling word wrap...\n\n");

	std::string ansi_info = 
		"Cursor Manipulation                               Clearing\n"
		"-------------------                               --------\n"
		"\\x1b[A    Cursor up                               \\x1b[2J    Clear screen and move cursor home\n"
		"\\x1b[B    Cursor down                             \\x1b[K     Clear to end of line\n"
		"\\x1b[C    Cursor right                            \\x1b[1K    Clear to beginning of line\n"
		"\\x1b[D    Cursor left                             \\x1b[3K    Clear entire line\n"
		"\\x1b[E    Cursor next line\n"
		"\\x1b[F    Cursor previous line\n"
		"\\x1b[G    Cursor horizontal absolute\n"
		"\\x1b[;H   Cursor position\n"
		"\\x1b[R    Report cursor position\n"
		"\\x1b[s    Save cursor position\n"
		"\\x1b[u    Restore cursor position\n"
		"\n"
		"Scrolling                                        Keyboard Input Modes\n"
		"---------                                        ---------------------\n"
		"\\x1b[S   Scroll up                              \\x1b[I     Focus gained\n"
		"\\x1b[T   Scroll down                            \\x1b[O     Focus lost\n";

	std::regex bracket_re(R"(\[([0-9;]*[A-Za-z]))");

	ansi_info = std::regex_replace(ansi_info, bracket_re, green + "[$1" + reset);
	io::replace_all(ansi_info, "\\x1b", cyan + "\\x1b" + reset);	

	io::print(ansi_info);
}



#pragma endregion

class Ansi : public Command {
	public:
		Ansi() : Command("ansi", "", "") {};

	int exec(std::vector<std::string> args) {
		if(args.empty()) {
			print_default();
			return 0;
		}

		decltype(args) validArgs = {
			"-f", "-n", "--foreground", "--non-colour", "--non-color", "--help"
		};

		for(auto& arg : args) {
			if(!io::vecContains(validArgs, arg)) {
				info::error("Invalid argument \"" + arg + "\"");
				return EINVAL;
			}

			if(arg == "--help") {
				io::print(R"(ansi: display different ansi codes, either for learning or reminders
usage: ansi [flag]

flags:
  -f | --foreground:   prints the ansi foreground color codes
  -n | --non-colo[u]r: prints a list of ansi codes used for terminal manipulation
)");
				return 0;
			}

			if(arg == "-f" || arg == "--foreground") {
				print_foreground();
				return 0;
			}

			if(arg == "-n" || arg == "--non-colour" || arg == "--non-color") {
				print_non_color();
				return 0;
			}
		}
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
