#ifndef SLASH_PYTHON_H
#define SLASH_PYTHON_H

#include <string>
#include <vector>
#include <boost/regex.hpp>
#include "helper.h"
#include "colors.h"
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

    boost::regex quotes(R"("""[\s\S]*?"""|'''[\s\S]*?'''|"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')");
    boost::regex comments(R"(#.*$)");
    boost::regex decorators(R"(@[A-Za-z_][A-Za-z0-9_]*)");
    boost::regex functions(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
    boost::regex variables(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\s*=))");
    boost::regex numbers(R"(0b[01]+|0x[0-9A-Fa-f]+|\b\d+(\.\d+)?\b)");
    boost::regex kwds("\\b(" + io::join(keywords, "|") + ")\\b");
    boost::regex vars(R"(\b[A-Za-z_][A-Za-z0-9_]*\b(?=\s*=))");


    const std::string blue = "\033[38;2;80;120;200m";
    const std::string gray = "\033[38;2;128;128;128m";

    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {vars, COLOR_VARS},
        {numbers, COLOR_NUMS},
        {kwds, COLOR_KEYWORDS},
        {functions, COLOR_FUNCS},
        {decorators, COLOR_ANNOTATIONS},
        {comments, COLOR_COMMENTS},
        {quotes, COLOR_QUOTES}
    };

    return shighlight(code, patterns);
}

#endif // SLASH_PYTHON_H
