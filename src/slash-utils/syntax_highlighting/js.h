#ifndef SLASH_JS_H
#define SLASH_JS_H

#include "../../abstractions/iofuncs.h"
#include "colors.h"
#include "helper.h"
#include <boost/regex.hpp>
#include <string>
#include <vector>

std::string js_sh(std::string content) {
    std::vector<std::string> js_keywords = {
        "break", "case", "catch", "class", "const", "continue", "debugger", "default",
        "delete", "do", "else", "export", "extends", "finally", "for", "function",
        "if", "import", "in", "instanceof", "let", "new", "return", "super", "switch",
        "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",
        "enum", "await", "implements", "interface", "package", "private", "protected",
        "public", "static"
    };

    boost::regex nums(R"(0x[0-9a-fA-F]+|0o[0-7]+|0b[01]+|\d+(\.\d*)?([eE][+-]?\d+)?)");
    boost::regex keywords("\\b(" + io::join(js_keywords, "|") + ")\\b");
    boost::regex constants(R"(\b(true|false|undefined|NaN|Infinity)\b)");
    boost::regex quotes(R"("([^"\\]|\\.)*"|'([^'\\]|\\.)*'|`([^`\\]|\\.)*`)");
    boost::regex funcs(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\())");
    boost::regex vars_in_quotes(R"(\$\{(?:[A-Za-z0-9._]+)\})");
    boost::regex comments(R"(\/\/.*)");
    boost::regex variables(R"((?<=let )[A-Za-z0-9_]+|(?<=const )[A-Za-z0-9_]+|(?<=var )[A-Za-z0-9_]+|([A-Za-z0-9_]+(?= =)|[A-Za-z0-9_]+(?= ==)|[A-Za-z0-9_]+(?= ==)|[A-Za-z0-9_]+(?= !=)|[A-Za-z0-9_]+(?= !==)))");
    boost::regex packages(R"((?<=import )[A-Za-z0-9_*]+|(?<=as )[A-Za-z0-9_*]+)");
    boost::regex classes(R"((?<=class )[A-Za-z0-9_]+|(?<=extends )[A-Za-z0-9_]+|(?<=new )[A-Za-z0-9_]+|\b(console|window|String|Number|Boolean|Symbol|BigInt|Object|Function|Array|Map|Set|WeakMap|WeakSet|Date|RegExp|Error|EvalError|RangeError|ReferenceError|SyntaxError|TypeError|URIError|Promise|Math|JSON|Intl|ArrayBuffer|DataView)\b)");

    // Prepare vector of pairs (regex, color)
    std::vector<std::pair<boost::regex, std::string>> patterns = {
        {nums, COLOR_NUMS},
        {keywords, COLOR_KEYWORDS},
        {classes, COLOR_CLASSES},
        {packages, COLOR_PACKAGES},
        {funcs, COLOR_FUNCS},
        {comments, COLOR_COMMENTS},
        {quotes, COLOR_QUOTES},
        {vars_in_quotes, COLOR_VAR_INSIDE_QUOTES},
        {constants, COLOR_CONSTANTS},
        {variables, COLOR_VARS},
    };

    return shighlight(content, patterns);
}

#endif // SLASH_JS_H
