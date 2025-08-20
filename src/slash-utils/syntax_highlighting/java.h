#ifndef SLASH_JAVA_H
#define SLASH_JAVA_H

#include "../../abstractions/iofuncs.h"
#include "colors.h"
#include "helper.h"
#include <boost/regex.hpp>
#include <string>
#include <vector>

std::string java_sh(std::string content) {
    std::vector<std::string> java_keywords = {
        "abstract", "assert", "boolean", "break", "byte", "case", "catch",
        "char", "class", "const", "continue", "default", "do", "double",
        "else", "enum", "extends", "final", "finally", "float", "for",
        "goto", "if", "implements", "import", "instanceof", "int",
        "interface", "long", "native", "new", "package", "private",
        "protected", "public", "return", "short", "static", "strictfp",
        "super", "switch", "synchronized", "this", "throw", "throws",
        "transient", "try", "void", "volatile", "while", "_"
    };

    boost::regex nums(R"(0b[01]+|0x[A-Ha-h0-9]+|\d+(\.\d+)?([eE][+-]?\d+)?[fFdD]?)");
    boost::regex keywords("\\b(" + io::join(java_keywords, "|") + ")\\b");
    boost::regex quotes(R"("(?:[^"\\]|\\.)*"|'(?:[^"\\]|\\.)*')");
    boost::regex constants(R"(\b(true|false|null)\b)");
    boost::regex common_types(R"(\b(int|boolean|double|byte|short|long|float|char|void)\b)");
    boost::regex annotations(R"(@[0-9A-Za-z_.]+)");
    boost::regex funcs(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
    boost::regex packages(R"((?<=import\s)[A-Za-z0-9_.]+\*?)");
    boost::regex comments(R"(\/\/.*)");

    // Prepare vector of pairs (regex, color)
    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {nums, COLOR_NUMS},
        {keywords, COLOR_KEYWORDS},
        {funcs, COLOR_FUNCS},
        {annotations, COLOR_ANNOTATIONS},
        {packages, COLOR_PACKAGES},
        {constants, COLOR_CONSTANTS},
        {quotes, COLOR_QUOTES},
        {comments, COLOR_COMMENTS},
    };

    return shighlight(content, patterns);
}

#endif // SLASH_JAVA_H
