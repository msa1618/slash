#include "parser.h"
#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../builtin-cmds/var.h"
#include "../builtin-cmds/alias.h"
#include <regex>

std::string bracket_to_ansi(std::string content) {
    io::replace_all(content, "(red)", red);
    io::replace_all(content, "(green)", green);
    io::replace_all(content, "(yellow)", yellow);
    io::replace_all(content, "(blue)", blue);
    io::replace_all(content, "(magenta)", magenta);
    io::replace_all(content, "(cyan)", cyan);
    io::replace_all(content, "(white)", white);

    io::replace_all(content, "(bg_red)", "\x1b[41m");
    io::replace_all(content, "(bg_green)", "\x1b[42m");
    io::replace_all(content, "(bg_yellow)", "\x1b[43m");
    io::replace_all(content, "(bg_blue)", "\x1b[44m");
    io::replace_all(content, "(bg_magenta)", "\x1b[45m");
    io::replace_all(content, "(bg_cyan)", "\x1b[46m");
    io::replace_all(content, "(bg_white)", "\x1b[47m");

    io::replace_all(content, "(bold)", bold);
    io::replace_all(content, "(italic)", "\x1b[3m");
    io::replace_all(content, "(underline)", "\x1b[4m");
    io::replace_all(content, "(blink)", "\x1b[5m");
    io::replace_all(content, "(strikethrough)", "\x1b[9m");

    io::replace_all(content, "(reset)", reset);

    return content;
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

		// Start of quoted string
		if ((c == '"' || c == '\'') && !dq_mode && !sq_mode) {
			if (i > 0 && parsed_command[i - 1] == '@') {
				cq_mode = true;
				// remove @ from buffer if it slipped in
				if (!buffer.empty() && buffer.back() == '@')
					buffer.pop_back();
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
			args.push_back(buffer);
			buffer.clear();
			dq_mode = false;
			sq_mode = false;
			continue;
		}

		// Escapes
		if (c == '\\' && next != '\0') {
			switch (next) {
				case '"': buffer.push_back('\"'); break;
				case '\'': buffer.push_back('\''); break;
				case 'n': buffer.push_back('\n'); break;
				case 't': buffer.push_back('\t'); break;
				case '\\': buffer.push_back('\\'); break;
				default:
					buffer.push_back(c);
					buffer.push_back(next);
					break;
			}
			i++; // skip next char
			continue;
		}

		// Argument break
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
