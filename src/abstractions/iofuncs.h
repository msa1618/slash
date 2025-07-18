#ifndef AMBER_IOFUNCS_H
#define AMBER_IOFUNCS_H

#include <unistd.h>
#include "definitions.h"
#include <cstring>
#include <string>
#include <vector>
#include <fcntl.h>
#include <variant>

namespace io {
	void print(std::string text);
	void print_err(const char* text);


	std::string join(std::vector<std::string> vec, std::string joiner);
	std::vector<std::string> split(const std::string &s, char delimiter);
	std::string center(std::string text, int width);

	std::string trim(const std::string& str);

	std::variant<std::string, int> read_file(std::string filepath);
	int write_to_file(std::string filepath, std::string content);
	int overwrite_file(std::string filepath, std::string content);

	template <typename T, typename U>
	bool vecContains(std::vector<T> vec, U target) { // For string vectors and string literals, which are interpreted as const char*
		for(auto& elm : vec) {
		if(elm == target) return true;
		}

		return false;
	};

	template <typename T>
	bool vecContains(std::vector<T> vec, T target) {
		for(auto& elm : vec) {
			if(elm == target) return true;
		}

		return false;
	};
}

#endif //AMBER_IOFUNCS_H
