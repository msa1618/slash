#include "parser.h"
#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../builtin-cmds/var.h"
#include "../builtin-cmds/alias.h"
#include <regex>

std::string remove_comments_outside_quotes(const std::string& input) {
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote) {
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
		char next = parsed_command[i + 1];

		if (c == ' ' && !buffer.empty() && !(dq_mode || sq_mode)) {
			args.push_back(buffer);
			buffer.clear();
			continue;
		}

		if (c == '"' && buffer.empty() && !(dq_mode || sq_mode)) {
			dq_mode = true;
			continue;
		}
		if (c == '"' && dq_mode && !sq_mode) {
			dq_mode = false;
			args.push_back(buffer);
			buffer.clear();
			continue;
		}

		if (c == '\'' && buffer.empty() && !(dq_mode || sq_mode)) {
			sq_mode = true;
			continue;
		}
		if (c == '\'' && sq_mode && !dq_mode) {
			sq_mode = false;
			args.push_back(buffer);
			buffer.clear();
			continue;
		}

		if(c == '\\' && i + 1 < parsed_command.size()) {
			switch(next) {
				case '"': buffer.push_back('\"'); i++; break;
				case '\'': buffer.push_back('\''); i++; break;
				case 'n': buffer.push_back('\n'); i++; break;
				case 't': buffer.push_back('\t'); i++; break;
				case '\\': buffer.push_back('\\'); i++; break;
				default:
					buffer.push_back(c);
					buffer.push_back(next);
					break;
			}
			continue;
		}

		buffer.push_back(c);
	}

	if (!buffer.empty()) {
		args.push_back(buffer);
	}

	// Variable substitution
	for (auto& arg : args) {
		size_t start = arg.find("$(");
		if (start != std::string::npos) {
			size_t end = arg.find(")", start);
			if (end != std::string::npos) {
				std::string variable = arg.substr(start + 2, end - (start + 2));
				if (variable.find(" ") != std::string::npos) {
					info::error("Variable cannot contain spaces!");
					return {};
				}
				auto value = get_value(variable);
				if (std::holds_alternative<std::string>(value)) {
					arg.replace(start, end - start + 1, std::get<std::string>(value));
				}
			}
		}
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
