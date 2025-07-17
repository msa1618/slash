#include <iostream>
#include <sstream>
#include "iofuncs.h"
#include "definitions.h"

#pragma once
#include <string>

namespace info {
	void warning(std::string content, std::string file_affected = "");
	void error(std::string error, int code = 0, std::string file_affected = "");
	void debug(std::string content);
}
