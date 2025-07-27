#include <sstream>
#include "iofuncs.h"
#include "definitions.h"
#include "info.h"

namespace info {
	void warning(std::string content, std::string file_affected) {
		std::stringstream ss;
		ss << yellow << "[Warning] " << reset << content;
		if (!file_affected.empty()) {
			ss << " - " << file_affected;
		}
		ss << '\n';
		io::print(ss.str());
	}

	void error(std::string error, int code, std::string file_affected) {
		std::stringstream ss;
		ss << red << "[Error] " << reset << error << " ";
		if (code != 0) {
			ss << gray << "(" << code << ") " << reset;
		}
		if (!file_affected.empty()) {
			ss << "- " << file_affected;
		}
		ss << '\n';
		io::print_err(ss.str());
	}

	void debug(std::string content) {
		std::stringstream ss;
		ss << blue << "[Debug] " << reset << content << " ";
		ss << '\n';
		io::print(ss.str());
	}
}
