#ifndef SLASH_GO_H
#define SLASH_GO_H

#include "../../abstractions/iofuncs.h"
#include "colors.h"
#include "helper.h"
#include <boost/regex.hpp>
#include <string>
#include <vector>

std::string go_sh(std::string content) {
    std::vector<std::string> go_keywords = {
        "break", "default", "func", "interface", "select",
        "case", "defer", "go", "map", "struct",
        "chan", "else", "goto", "package", "switch",
        "const", "fallthrough", "if", "range", "type",
        "continue", "for", "import", "return", "var"
    };

    boost::regex nums(R"(0b[01_]+|0o[0-7_]+|0x[0-9a-fA-F_]+(\.[0-9a-fA-F_]*)?(p[+-]?[0-9_]+)?|\d+(_?\d+)*(\.\d*(_?\d+)*)?([eE][+-]?\d+)?)");
    boost::regex keywords("\\b(" + io::join(go_keywords, "|") + ")\\b");
    boost::regex constants(R"(\b(true|false|iota|nil)\b)");
    boost::regex quotes(R"("(?:[^"\\]|\\.)*"|'(?:[^"\\]|\\.)*')");
    boost::regex funcs(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
    boost::regex comments(R"(\/\/.*)");
    boost::regex variables(R"([A-Za-z0-9_]+(?= =)|[A-Za-z0-9_]+(?= :=)|[A-Za-z0-9_]+(?= !=)|[A-Za-z0-9_]+(?= ==))");
    boost::regex packages(R"((?<=package )[A-Za-z0-9_]+)");
    boost::regex types(R"(\b(bool|byte|complex64|complex128|error|float32|float64|int|int8|int16|int32|int64|rune|string|uint|uint8|uint16|uint32|uint64|uintptr)\b)");

    // Prepare vector of pairs (regex, color)
    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {nums, COLOR_NUMS},
        {keywords, COLOR_KEYWORDS},
        {funcs, COLOR_FUNCS},
        {packages, COLOR_PACKAGES},
        {types, COLOR_TYPES},
        {comments, COLOR_COMMENTS},
        {quotes, COLOR_QUOTES},
        {constants, COLOR_CONSTANTS},
        {variables, COLOR_VARS}
    };

    return shighlight(content, patterns);
}

#endif // SLASH_GO_H
