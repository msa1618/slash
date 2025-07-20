#ifndef AMBER_PYTHON_H
#define AMBER_PYTHON_H

#include <string>
#include <vector>
#include <regex>
#include "helper.h"
#include "../../abstractions/definitions.h"
#include "../../abstractions/iofuncs.h"

std::string python_sh(std::string code) {
	std::vector<std::string> keywords = {
		"False", "None", "True", "and", "as", "assert", "async", "await",
		"break", "class", "continue", "def", "del", "elif", "else", "except",
		"finally", "for", "from", "global", "if", "import", "in", "is",
		"lambda", "nonlocal", "not", "or", "pass", "raise", "return",
		"try", "while", "with", "yield", "match", "case"
	};

	std::regex quotes(R"(".*")");
	std::regex comments(R"(#.*)");
	std::regex decorators(R"(@[A-Za-z0-9_-]+)");
	std::regex functions(R"([A-Za-z0-9-_]+(?=\())");
	std::regex variables(R"([A-Za-z0-9-_]+(?=\s=))");
	std::regex numbers(R"(0b[01]+|0x[0-9A-Fa-f]+|[0-9]+)");
	std::regex kwds(std::string(R"(\b()") + io::join(keywords, "|") + R"()\b)");

	std::string highlighted = highlight(code, numbers, "\033[38;2;236;157;237m");
	highlighted = highlight(highlighted, kwds, "\033[38;2;39;125;161m");
	highlighted = highlight(highlighted, functions, "\033[38;2;255;159;28m");
	highlighted = highlight(highlighted, decorators, blue);
	highlighted = highlight(highlighted, comments, gray);
	highlighted = highlight(highlighted, quotes, "\033[38;2;103;148;54m");

	return highlighted;
}

#endif //AMBER_PYTHON_H
