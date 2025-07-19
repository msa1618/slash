#include <chrono>
#include <string>
#include <locale>
#include <iostream>
#include <iomanip>
#include "../abstractions/iofuncs.h"
#include "../command.h"

class Datetime : public Command {
	private:
		void print_locale_time() {
			std::time_t time = std::time(nullptr);
			std::tm* localtime = std::localtime(&time);

			std::cout.imbue(std::locale("")); // Use system locale
			std::cout << std::put_time(localtime, "%c") << "\n"; // %c means locale datetime
		}

		void print_datetime_flags() {

		}

	public:
		Datetime() : Command("datetime", "", "") {}

		int exec(std::vector<std::string> args) {
			if(args.empty()) {
				print_locale_time();
				return 0;
			}
		}
};

int main(int argc, char* argv[]) {
	Datetime datetime;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	datetime.exec(args);
	return 0;
}
