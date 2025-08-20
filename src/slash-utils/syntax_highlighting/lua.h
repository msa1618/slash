#ifndef SLASH_LUA_H
#define SLASH_LUA_H

#include "../../abstractions/iofuncs.h"
#include "colors.h"
#include "helper.h"
#include <boost/regex.hpp>
#include <string>
#include <vector>

std::string lua_sh(std::string content) {
    std::vector<std::string> lua_keywords = {
        "and", "break", "do", "else", "elseif", "end", "for",
        "function", "goto", "if", "in", "local", "not", "or",
        "repeat", "return", "then", "until", "while"
    };

    boost::regex nums(R"(0x[0-9a-fA-F]+(\.[0-9a-fA-F]*)?(p[+-]?[0-9]+)?|\d+(\.\d*)?([eE][+-]?\d+)?)");
    boost::regex keywords("\\b(" + io::join(lua_keywords, "|") + ")\\b");
    boost::regex constants(R"(\b(true|false|nil)\b)");
    boost::regex quotes(R"("(?:[^"\\]|\\.)*"|'(?:[^"\\]|\\.)*')");
    boost::regex funcs(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
    boost::regex comments(R"(--.*)");
    boost::regex variables(R"((?<=local )[A-Za-z0-9_]+|[A-Za-z0-9_]+(?= \=)|[A-Za-z0-9_]+(?=\=))");

    // Prepare vector of pairs (regex, color)
    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {nums, COLOR_NUMS},
        {keywords, COLOR_KEYWORDS},
        {funcs, COLOR_FUNCS},
        {comments, COLOR_COMMENTS},
        {quotes, COLOR_QUOTES},
        {constants, COLOR_CONSTANTS},
        {variables, COLOR_VARS}
    };

    return shighlight(content, patterns);
}

#endif // SLASH_LUA_H
