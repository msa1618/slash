#ifndef SLASH_RUST_H
#define SLASH_RUST_H

#include <string>
#include <vector>
#include <boost/regex.hpp>
#include "../../abstractions/iofuncs.h"
#include "helper.h"

std::string rust_sh(std::string content) {
    std::vector<std::string> rust_keywords = {
      "as", "break", "const", "continue", "crate", "else", "enum",
      "extern", "false", "fn", "for", "if", "impl", "in", "let",
      "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
      "self", "Self", "static", "struct", "super", "trait", "true",
      "type", "unsafe", "use", "where", "while", "async", "await",
      "dyn"
    };

    boost::regex nums(R"(0b[01]+|0x[A-Ha-h0-9]+|\d+(\.\d+)?([eE][+-]?\d+)?[fFdD]?)");
    boost::regex keywords("\\b(" + io::join(rust_keywords, "|") + ")\\b");
    boost::regex quotes(R"("(?:[^"\\]|\\.)*"|'(?:[^"\\]|\\.)*')");
    boost::regex funcs(R"(\b[A-Za-z0-9_!]*(?=\())");
    boost::regex macro_rules(R"(macro_rules!)");
    boost::regex types(R"(\b(i8|i16|i32|i64|i128|isize|u8|u16|u32|u64|u128|usize|f32|f64|bool|char|str)\b)");
    boost::regex attrs(R"(#\!?(\[.*?\]))");
    boost::regex namespaces(R"([A-Za-z0-9_]+(?=::))");
    boost::regex cmts(R"(\/\/.*)");

    // Prepare vector of pairs (regex, color)
    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {nums, "\x1b[38;2;236;157;237m"},
        {keywords, "\x1b[38;2;39;125;161m"},
        {funcs, "\x1b[38;2;255;159;28m"},
        {quotes, "\x1b[38;2;103;148;54m"},
        {cmts, "\x1b[38;2;88;129;87m"},
        {attrs, gray},
        {types, magenta},
        {namespaces, "\x1b[38;2;179;182;10m"},
        {macro_rules, yellow}
    };

    return shighlight(content, patterns);
}

#endif // SLASH_RUST_H