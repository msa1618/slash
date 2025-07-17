#include <string>
#include <sstream>
#include <random>
#include "../command.h"
#include "../abstractions/iofuncs.h"

#pragma region helpers

std::string to_uppercase(std::string& text) {
	for(auto& c : text) {
		c = toupper(c);
	}
	return text;
}

std::string to_lowercase(std::string& text) {
	for(auto& c : text) {
		c = tolower(c);
	}
	return text;
}

std::string to_weirdcase(std::string& text) {
	std::string output;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 1);

	for(char c : text) {
		if(std::isalpha(c)) {
			if(dis(gen) == 0) output += std::tolower(c);
			else output += std::toupper(c);
		} else {
			output += c;
		}
	}

	return output;
}

void set_ansi_rgb(int r, int g, int b) {
	std::stringstream result;
	result << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
	io::print(result.str());
}

void rainbow_text(std::string text) {
	int length = text.size();

	for (int i = 0; i < length; ++i) {
		double ratio = (double)i / length;
		// Use sine waves to get smooth rainbow colors
		int r = (int)(std::sin(ratio * 2 * M_PI) * 127 + 128);
		int g = (int)(std::sin(ratio * 2 * M_PI + 2 * M_PI / 3) * 127 + 128);
		int b = (int)(std::sin(ratio * 2 * M_PI + 4 * M_PI / 3) * 127 + 128);

		set_ansi_rgb(r, g, b);
		io::print(std::string(1, text[i]));
	}
	io::print(reset);
}



#pragma endregion

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
										 "-r: print in rainbow",
										 "") {}

	int exec(std::vector<std::string> args) override {
		std::vector<std::string> text;
		bool infinite = false;

		bool loop = false;
		int loopCount = 0;

		bool uppercase = false;
		bool lowercase = false;
		bool weirdcase = false;
		bool use_rainbow = false;

		std::string colorCode;

		for (size_t i = 0; i < args.size(); ++i) {
			if (!args[i].empty() && args[i][0] == '-') {
				if (args[i] == "-i") {
					infinite = true;
				} else if (args[i] == "-lp" && i + 1 < args.size()) {
					loopCount = std::stoi(args[++i]);
					loop = true;
				} else if (args[i] == "-u") {
					uppercase = true;
				} else if(args[i] == "-l") {
					lowercase = true;
				} else if(args[i] == "-w") {
					weirdcase = true;
				} else if(args[i] == "-r") {
					use_rainbow = true;
				}
			} else {
				text.push_back(args[i]);
			}
		}

		if(uppercase) { for(auto& word : text) word = to_uppercase(word); }
		if(lowercase) { for(auto& word : text) word = to_lowercase(word); }
		if(weirdcase) { for(auto& word : text) word = to_weirdcase(word); }

		std::stringstream ss;
		for (const auto& word : text) ss << word << " ";
		std::string str = ss.str();

		if (infinite) {
			while (true) io::print(str);
		}
		else if(use_rainbow) {
			rainbow_text(str);
			io::print("\n");
		}
		else if(loop) {
			for (int i = 0; i < loopCount; ++i) {
				io::print(str);
				io::print("\n");
			}
		}
		else {
			io::print(str);
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