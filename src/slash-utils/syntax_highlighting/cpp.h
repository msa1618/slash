#ifndef AMBER_CPP_H
#define AMBER_CPP_H

#include "../../abstractions/colors.h"
#include "../../abstractions/iofuncs.h"
#include <regex>
#include <string>
#include <vector>

std::string highlight(const std::string& input, std::regex pattern, std::string color) {
	std::string result;
	std::string::const_iterator searchStart(input.cbegin());
	std::smatch match;

	while (std::regex_search(searchStart, input.cend(), match, pattern)) {
		result.append(searchStart, match[0].first);
		result.append(color + match.str() + "\033[0m");
		searchStart = match[0].second;
	}

	result.append(searchStart, input.cend());

	return result;
}

std::string cpp_sh(std::string content) {
	std::vector<std::string> cpp_keywords = {
		"alignas", "alignof", "and", "and_eq", "asm", "auto",
		"bitand", "bitor", "bool", "break", "case", "catch",
		"char", "char8_t", "char16_t", "char32_t", "class",
		"compl", "concept", "const", "consteval", "constexpr",
		"constinit", "const_cast", "continue", "co_await", "co_return",
		"co_yield", "decltype", "default", "delete", "do", "double",
		"dynamic_cast", "else", "enum", "explicit", "export", "extern",
		"false", "float", "for", "friend", "goto", "if", "inline",
		"int", "long", "mutable", "namespace", "new", "noexcept",
		"not", "not_eq", "nullptr", "operator", "or", "or_eq", "override",
		"private", "protected", "public", "register", "reinterpret_cast",
		"requires", "return", "short", "signed", "sizeof", "static",
		"static_assert", "static_cast", "struct", "switch", "synchronized",
		"template", "this", "thread_local", "throw", "true", "try",
		"typedef", "typeid", "typename", "union", "unsigned", "using",
		"virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
	};

	std::vector<std::string> preprocessor_directives = {
		"define",
		"undef",
		"include",
		"if",
		"ifdef",
		"ifndef",
		"else",
		"elif",
		"endif",
		"error",
		"pragma",
		"line",
		"warning"
	};

	std::regex nums(R"(-?\d+(\.\d+)?)");
	// std::regex escape_chars(R"(\([A-Za-z]|[0-9]|[\"\']))"); // suspicious, comment out or fix

	std::regex keywords("\\b(" + join(cpp_keywords, "|") + ")\\b");
	std::regex quotes(R"("(?:[^"\\]|\\.)*")");
	std::regex funcs(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
	std::regex comments(R"(//.*$)", std::regex_constants::multiline);
	std::regex namespaces(R"([A-Za-z0-9_]+(?=::))");
	std::regex prep_directives("#(" + io::join(preprocessor_directives, "|") + ")");

	std::string highlighted = highlight(content, nums, "\x1b[38;2;236;157;237m");
	highlighted = highlight(highlighted, keywords, "\x1b[38;2;39;125;161m");
	highlighted = highlight(highlighted, funcs, "\x1b[38;2;255;159;28m");
	highlighted = highlight(highlighted, namespaces, "\x1b[38;2;179;182;10m");
	highlighted = highlight(highlighted, prep_directives, "\x1b[38;2;165;56;96m");
	highlighted = highlight(highlighted, quotes, "\x1b[38;2;103;148;54m");

	return highlighted;
}

#endif // AMBER_CPP_Hs
