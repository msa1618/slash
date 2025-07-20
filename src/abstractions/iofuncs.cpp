#include <sstream>
#include <math.h>
#include "iofuncs.h"
#include "info.h"
#include <unistd.h>
#include <cstring>
#include <variant>

void io::print(std::string text) {
	write(STDOUT, text.c_str(), text.length());
}

void io::print_err(const char *text) {
	write(STDERR, text, strlen(text));
}

std::vector<std::string> io::split(const std::string& s, std::string delimiter) {
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end;

	while ((end = s.find(delimiter, start)) != std::string::npos) {
		tokens.push_back(s.substr(start, end - start));
		start = end + delimiter.length();
	}
	tokens.push_back(s.substr(start));

	return tokens;
}


std::string io::join(std::vector<std::string> vec, std::string joiner) {
	std::stringstream ss;
	for(auto elm : vec) {
		ss << elm << joiner;
	}
	return ss.str();
}

void io::replace_all(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty()) return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

std::string io::center(std::string text, int width) {
	int pad = ceil(width / 4);

	return std::string(pad, ' ') + text + std::string(pad, ' ');
}

std::variant<std::string, int> io::read_file(std::string filepath) {
	int fd = open(filepath.c_str(), O_RDONLY);
	if(fd == -1) {
		return errno;
	}

	char buffer[1024];
	ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
	if(bytesRead < 0) {
		return errno;
	}

	buffer[bytesRead] = '\0';
	std::string content = buffer;
	close(fd);
	return content;
}

int io::write_to_file(std::string filepath, std::string content) {
	int fd = open(filepath.c_str(), O_WRONLY | O_APPEND);
	if(fd == -1) {
		info::error(strerror(errno), errno, filepath);
		return -1;
	}

	ssize_t bytes_written = write(fd, content.c_str(), content.length());
	if(bytes_written == -1) {
		info::error(strerror(errno), errno, filepath);
	}

	close(fd);
	return 0;
}

int io::overwrite_file(std::string filepath, std::string content) {
	int fd = open(filepath.c_str(), O_WRONLY | O_TRUNC);
	if(fd == -1) {
		info::error(strerror(errno), errno, filepath);
		return -1;
	}

	ssize_t bytes_written = write(fd, content.c_str(), content.length());
	if(bytes_written == -1) {
		info::error(strerror(errno), errno, filepath);
	}

	close(fd);
	return 0;
}

std::string io::trim(const std::string &str) {
	const std::string whitespace = " \t\n\r\f\v";
	size_t start = str.find_first_not_of(whitespace);
	if (start == std::string::npos) return "";
	size_t end = str.find_last_not_of(whitespace);
	return str.substr(start, end - start + 1);
}