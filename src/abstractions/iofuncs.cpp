#include <sstream>
#include <math.h>
#include "iofuncs.h"
#include <unistd.h>
#include <cstring>

void io::print(std::string text) {
	write(STDOUT, text.c_str(), text.length());
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

std::string io::read_file(std::string filepath) {
	int fd = open(filepath.c_str(), O_RDONLY);
	if(fd == -1) {
		return "";
	}

	char buffer[1024];
	ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
	if(bytesRead < 0) {
		return "";
	}

	buffer[bytesRead] = '\0';
	std::string content = buffer;
	close(fd);
	return content;
}

void io::write_to_file(std::string filepath, std::string content) {
	int fd = open(filepath.c_str(), O_WRONLY | O_APPEND);
	if(fd == -1) { perror("error"); return; }

	ssize_t bytes_written = write(fd, content.c_str(), content.length());
	if(bytes_written == -1) { perror("error"); return; }

	close(fd);
}