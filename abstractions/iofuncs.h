#ifndef AMBER_IOFUNCS_H
#define AMBER_IOFUNCS_H

#include <unistd.h>
#include "definitions.h"
#include <cstring>
#include <string>
#include <vector>

namespace io {
	void print(const char* text);
	void print_err(const char* text);

	std::string join(std::vector<std::string> vec, std::string joiner);
	std::vector<std::string> split(const std::string &s, char delimiter);
	std::string center(std::string text, int width);

	template <typename T>
	bool vecContains(std::vector<T> vec, T target) {
		for(auto& elm : vec) {
		if(elm == target) return true;
		}

		return false;
	};
}

#endif //AMBER_IOFUNCS_H
