#ifndef SLASH_CPP_H
#define SLASH_CPP_H

#include "../../abstractions/iofuncs.h"
#include "helper.h"
#include "colors.h"
#include <boost/regex.hpp>
#include <string>
#include <vector>

std::string cpp_sh(std::string content) {
    std::vector<std::string> cpp_keywords = {
        "alignas", "alignof", "and", "and_eq", "asm",
        "bitand", "bitor", "break", "case", "catch",
        "class", "compl", "concept", "const", "consteval", "constexpr",
        "constinit", "const_cast", "continue", "co_await", "co_return",
        "co_yield", "decltype", "default", "delete", "do",
        "dynamic_cast", "else", "enum", "explicit", "export", "extern",
        "for", "friend", "goto", "if", "inline",
        "mutable", "namespace", "new", "noexcept",
        "not", "not_eq", "operator", "or", "or_eq", "override",
        "private", "protected", "public", "register", "reinterpret_cast",
        "requires", "return", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "synchronized",
        "template", "this", "thread_local", "throw", "try",
        "typedef", "typeid", "typename", "union", "using",
        "virtual", "volatile", "while", "xor", "xor_eq"
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

    boost::regex nums(R"(0b[01]+|0x[A-Ha-h0-9]+|\d+(\.\d+)?([eE][+-]?\d+)?[fFdD]?)");
    boost::regex types("\\b(auto|bool|char|signed char|unsigned char|wchar_t|char16_t|char32_t|int|short|long|long long|signed|unsigned|float|double|long double|void|int8_t|int16_t|int32_t|int64_t|uint8_t|uint16_t|uint32_t|uint64_t|size_t|ptrdiff_t|nullptr_t)\\b");
    boost::regex keywords("\\b(" + io::join(cpp_keywords, "|") + ")\\b");
    boost::regex constants(R"(\b(true|false)\b)");
    boost::regex attributes(R"(\[\[.*\]\])");
    boost::regex quotes(R"("(?:[^"\\]|\\.)*"|'(?:[^"\\]|\\.)*')");
    boost::regex funcs(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
    boost::regex comments(R"(\/\/.*)");
    boost::regex namespaces(R"([A-Za-z0-9_]+(?=::))");
    boost::regex prep_directives("#(" + io::join(preprocessor_directives, "|") + ")");

    // Prepare vector of pairs (regex, color)
    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {nums, COLOR_NUMS},
        {keywords, COLOR_KEYWORDS},
        {types, COLOR_TYPES},
        {constants, COLOR_CONSTANTS},
        {funcs, COLOR_FUNCS},
        {namespaces, COLOR_NAMESPACES},
        {quotes, COLOR_QUOTES},
        {comments, COLOR_COMMENTS},
        {attributes, COLOR_ATTRS},
        {prep_directives, COLOR_PREP_DIRECTIVES},
    };

    return shighlight(content, patterns);
}

#endif // SLASH_CPP_H
