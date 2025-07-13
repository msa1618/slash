#include <string>
#include <sstream>
#include "../command.h"

class Echo : public Command {
	public:
		Echo() : Command("echo",
										 "Outputs whatever you type in."
										 "Flags:"
										 "-u: Uppercase"
										 "-l: Lowercase"
										 "-w: wEiRdcAsE"
										 "-lp [num]: Loops [num] amount of times"
										 "-i: Loops infinitely."
										 "-c [code]: Sets the color. Use man echo to see the codes.",
										 "") {}

	int exec(std::vector<std::string> args) override {
		std::vector<std::string> text;
		bool infinite = false;
		int loopCount = 1;

		std::string colorCode;

		for (size_t i = 0; i < args.size(); ++i) {
			if (!args[i].empty() && args[i][0] == '-') {
				if (args[i] == "-i") {
					infinite = true;
				} else if (args[i] == "-lp" && i + 1 < args.size()) {
					loopCount = std::stoi(args[++i]);
				} else if (args[i] == "-c" && i + 1 < args.size()) {
					colorCode = args[++i];
				} else if (args[i] == "-u" || args[i] == "-l" || args[i] == "-w") {
					text.push_back(args[i]);
				}
			} else {
				text.push_back(args[i]);
			}
		}

		std::stringstream ss;
		for (const auto& word : text) ss << word << " ";
		std::string str = ss.str();

		if (std::find(args.begin(), args.end(), "-u") != args.end()) {
			for (char& c : str) c = toupper(c);
		}
		if (std::find(args.begin(), args.end(), "-l") != args.end()) {
			for (char& c : str) c = tolower(c);
		}
		if (std::find(args.begin(), args.end(), "-w") != args.end()) {
			srand(time(NULL));
			for (char& c : str) c = (rand() % 2 == 0) ? tolower(c) : toupper(c);
		}

		if (infinite) {
			while (true) io::print(str.c_str());
		} else {
			for (int i = 0; i < loopCount; ++i) {
				io::print(str.c_str());
				io::print("\n");
			}
		}

		return 0;
	}

};

int main(int argc, char* argv[]) {
	Echo echoObj;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	int ret = echoObj.exec(args);
	return ret;
}