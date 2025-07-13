#include <sstream>
#include <math.h>
#include "iofuncs.h"

void io::print(std::string text) {
	write(STDOUT, text.c_str(), strlen(text.c_str()));
}

void io::print_err(const char *text) {
	write(STDERR, text, strlen(text));
}

std::vector<std::string> io::split(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream ss(s);
	std::string item;

	while (std::getline(ss, item, delimiter)) {
		tokens.push_back(item);
	}

	return tokens;
}

std::string io::join(std::vector<std::string> vec, std::string joiner) {
	std::stringstream ss;
	for(auto elm : vec) {
		ss << elm << joiner;
	}
	return ss.str();
}

std::string io::center(std::string text, int width) {
	int pad = ceil(width / 4);

	return std::string(pad, ' ') + text + std::string(pad, ' ');
}