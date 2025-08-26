#include "parser.h"
#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../builtin-cmds/var.h"
#include "../builtin-cmds/alias.h"
#include <boost/regex.hpp>

std::string unescape(const std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[++i];
            switch (next) {
                case 'a': result.push_back('\a'); break;
                case 'b': result.push_back('\b'); break;
                case 'e': result.push_back('\x1b'); break;  // escape char
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'v': result.push_back('\v'); break;
                case '\\': result.push_back('\\'); break;
                case '\'': result.push_back('\''); break;
                case '"': result.push_back('"'); break;
                default:
                    result.push_back('\\');
                    result.push_back(next);
            }
        } else {
            result.push_back(input[i]);
        }
    }
    return result;
}
std::string bracket_to_ansi(std::string content) {
    io::replace_all(content, "(red)", "\x1b[31m");
    io::replace_all(content, "(green)", "\x1b[32m");
    io::replace_all(content, "(yellow)", "\x1b[33m");
    io::replace_all(content, "(blue)", "\x1b[34m");
    io::replace_all(content, "(magenta)", "\x1b[35m");
    io::replace_all(content, "(cyan)", "\x1b[36m");
    io::replace_all(content, "(white)", "\x1b[37m");

    io::replace_all(content, "(bred)", "\x1b[91m");
    io::replace_all(content, "(bgreen)", "\x1b[92m");
    io::replace_all(content, "(byellow)", "\x1b[93m");
    io::replace_all(content, "(bblue)", "\x1b[94m");
    io::replace_all(content, "(bmagenta)", "\x1b[95m");
    io::replace_all(content, "(bcyan)", "\x1b[96m");
    io::replace_all(content, "(bwhite)", "\x1b[97m");

    io::replace_all(content, "(bg_red)", "\x1b[41m");
    io::replace_all(content, "(bg_green)", "\x1b[42m");
    io::replace_all(content, "(bg_yellow)", "\x1b[43m");
    io::replace_all(content, "(bg_blue)", "\x1b[44m");
    io::replace_all(content, "(bg_magenta)", "\x1b[45m");
    io::replace_all(content, "(bg_cyan)", "\x1b[46m");
    io::replace_all(content, "(bg_white)", "\x1b[47m");

    io::replace_all(content, "(bg_bred)", "\x1b[101m");
    io::replace_all(content, "(bg_bgreen)", "\x1b[102m");
    io::replace_all(content, "(bg_byellow)", "\x1b[103m");
    io::replace_all(content, "(bg_bblue)", "\x1b[104m");
    io::replace_all(content, "(bg_bmagenta)", "\x1b[105m");
    io::replace_all(content, "(bg_bcyan)", "\x1b[106m");
    io::replace_all(content, "(bg_bwhite)", "\x1b[107m");

    io::replace_all(content, "(bold)", "\x1b[1m");
    io::replace_all(content, "(dim)", "\x1b[2m");
    io::replace_all(content, "(italic)", "\x1b[3m");
    io::replace_all(content, "(underline)", "\x1b[4m");
    io::replace_all(content, "(dunderline)", "\x1b[21m");
    io::replace_all(content, "(blink)", "\x1b[5m");
    io::replace_all(content, "(reverse)", "\x1b[7m");
    io::replace_all(content, "(conceal)", "\x1b[8m");
    io::replace_all(content, "(strikethrough)", "\x1b[9m");
    io::replace_all(content, "(framed)", "\x1b[51m");
    io::replace_all(content, "(encircled)", "\x1b[52m");
    io::replace_all(content, "(overline)", "\x1b[53m");

    io::replace_all(content, "(reset)", "\x1b[0m");

    boost::regex reg_256(R"(\(([0-9]+)\))");
    content = boost::regex_replace(content, reg_256, "\x1b[38;5;$1m");

    boost::regex reg_bg_256(R"(\(b([0-9]+)\))");
    content = boost::regex_replace(content, reg_bg_256, "\x1b[48;5;$1m");

    boost::regex reg_rgb(R"(\(rgb:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3})\))");
    content = boost::regex_replace(content, reg_rgb, "\x1b[38;2;$1;$2;$3m");

    boost::regex reg_bg_rgb(R"(\(brgb:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3})\))");
    content = boost::regex_replace(content, reg_bg_rgb, "\x1b[48;2;$1;$2;$3m");

    boost::regex reg_hex(R"(\(#([0-9A-Fa-f]{6})\))");
    std::string result;
    std::string::const_iterator start = content.begin();
    std::string::const_iterator end = content.end();
    boost::smatch match;

    auto hex_to_rgb = [](const std::string& hex, bool bg) {
        int r = std::stoi(hex.substr(0,2), nullptr, 16);
        int g = std::stoi(hex.substr(2,2), nullptr, 16);
        int b = std::stoi(hex.substr(4,2), nullptr, 16);

        std::stringstream ss;
        if(bg) ss << "\x1b[48;2;";
        else ss << "\x1b[38;2;";
        ss << r << ";" << g << ";" << b << "m";
        return ss.str();
    };

    while(boost::regex_search(start, end, match, reg_hex)) {
        result.append(start, match[0].first);
        result.append(hex_to_rgb(match[1], false));
        start = match[0].second;
    }
    result.append(start, end);
    content = result;

    boost::regex reg_bg_hex(R"(\(b#([0-9A-Fa-f]{6})\))");
    result.clear();
    start = content.begin();
    end = content.end();

    while(boost::regex_search(start, end, match, reg_bg_hex)) {
        result.append(start, match[0].first);
        result.append(hex_to_rgb(match[1], true));
        start = match[0].second;
    }
    result.append(start, end);

    return result;
}


std::string remove_comments_outside_quotes(const std::string& input) {
    bool in_single_quote = false;
    bool in_double_quote = false;

    bool color_quote_mode = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        char next;
        if(i + 1 < input.size()) next = input[i + 1];

        if (c == '\'' && !in_double_quote && !color_quote_mode) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote && !color_quote_mode) {
            in_double_quote = !in_double_quote;
        }

        if (!in_single_quote && !in_double_quote) {
            if (c == '#') {
                return input.substr(0, i);
            }
        }
    }

    return input;
}

Args parse_arguments(std::string command) {
  command = io::trim(command);
  command = remove_comments_outside_quotes(command);
  if(command.empty()) return {};

  std::string buffer;
  Args args;

  bool dq_mode = false;
  bool sq_mode = false;
  bool cq_mode = false;
  bool literal = false;

  // Detect and replace alias in the raw command string
  std::string first_word;
  size_t i = 0;
  while (i < command.size() && command[i] != ' ') {
    first_word += command[i++];
  }
  std::string alias = get_alias(first_word, false);
  std::string parsed_command = (alias != "UNKNOWN")
    ? alias + command.substr(first_word.size())
    : command;

  // Now parse parsed_command instead of original command
  buffer.clear();
  for (i = 0; i < parsed_command.size(); i++) {
    char c = parsed_command[i];
    char next = (i + 1 < parsed_command.size()) ? parsed_command[i + 1] : '\0';
    char prev = (i != 0) ? parsed_command[i - 1] : ' ';

    // Start of quoted string
    if ((c == '"' || c == '\'') && !dq_mode && !sq_mode) {
      if (i > 0 && parsed_command[i - 1] == '@') {
        cq_mode = true;
        if (!buffer.empty() && buffer.back() == '@') {
          buffer.pop_back();
        }
      }
      if (i > 0 && parsed_command[i - 1] == 'E') {
        literal = true;
        if (!buffer.empty() && buffer.back() == 'E') {
          buffer.pop_back();
        }
      }
      if (c == '"') dq_mode = true;
      else sq_mode = true;
      continue;
    }

    // End of quoted string
    if ((c == '"' && dq_mode) || (c == '\'' && sq_mode)) {
      if (cq_mode) {
        buffer = bracket_to_ansi(buffer);
        cq_mode = false;
      }
      if (literal) {
        int backslash_count = 0;
        int j = i - 1;
        while (j >= 0 && parsed_command[j] == '\\') {
            backslash_count++;
            j--;
        }
        if (backslash_count % 2 == 0) {  // quote NOT escaped
            buffer = unescape(buffer);
            literal = false;
            args.push_back(buffer);
            buffer.clear();
        } else {
            buffer.push_back(c);
            continue;
        }
      }

      args.push_back(buffer);
      buffer.clear();
      dq_mode = false;
      sq_mode = false;
      continue;
    }

    if (c == '$' && prev != '\\') {
        // Find variable name
        size_t var_start = i + 1;
        size_t var_end = var_start;
        while (var_end < parsed_command.size() &&
              (isalnum(parsed_command[var_end]) || parsed_command[var_end] == '_')) {
            var_end++;
        }
        std::string var_name = parsed_command.substr(var_start, var_end - var_start);

        auto val_variant = get_value(var_name);
        if (!std::holds_alternative<std::string>(val_variant)) {
            info::error("Variable does not exist!");
            return {};
        }

        buffer += std::get<std::string>(val_variant);
        i = var_end - 1;
        continue;
    }

    if (c == ' ' && !dq_mode && !sq_mode) {
      if (!buffer.empty()) {
        args.push_back(buffer);
        buffer.clear();
      }
      continue;
    }

    // Fallback
    buffer.push_back(c);
  }

  if (!buffer.empty()) {
    args.push_back(buffer);
  }

  return args;
}

std::vector<Args> parse_pipe_commands(const std::string& command) {
  std::vector<Args> result;
  Args pipe_cmds = io::split(command, "|");
  for (auto& cmd : pipe_cmds) {
    cmd = io::trim(cmd);
    result.push_back(parse_arguments(cmd));
  }
  return result;
}
