#ifndef SLASH_CPP_H
#define SLASH_CPP_H

#include "../../abstractions/iofuncs.h"
#include "helper.h"
#include <boost/regex.hpp>
#include <string>
#include <vector>

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

    boost::regex nums(R"(-?\d+(\.\d+)?)");
    boost::regex keywords("\\b(" + io::join(cpp_keywords, "|") + ")\\b");
    boost::regex quotes(R"("(?:[^"\\]|\\.)*")");
    boost::regex funcs(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
    boost::regex namespaces(R"([A-Za-z0-9_]+(?=::))");
    boost::regex prep_directives("#(" + io::join(preprocessor_directives, "|") + ")");

    // Prepare vector of pairs (regex, color)
    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {nums, "\x1b[38;2;236;157;237m"},
        {keywords, "\x1b[38;2;39;125;161m"},
        {funcs, "\x1b[38;2;255;159;28m"},
        {namespaces, "\x1b[38;2;179;182;10m"},
        {prep_directives, "\x1b[38;2;165;56;96m"},
        {quotes, "\x1b[38;2;103;148;54m"}
    };

    return shighlight(content, patterns);
}

#endif // SLASH_CPP_H
