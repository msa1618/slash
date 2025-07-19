#include "parser.h"
#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../builtin-cmds/var.h"

Args parse_arguments(const std::string& command) {
	std::string buffer;
	Args args;

	bool dq_mode = false;
	bool sq_mode = false;

	for (size_t i = 0; i < command.size(); i++) {
		char c = command[i];

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

		buffer.push_back(c);
	}

	for(auto& arg : args) {
		if (arg.find("$(") != std::string::npos && arg.find(")", arg.find("$(")) != std::string::npos) {
			std::size_t start = arg.find("$(");
			std::size_t end = arg.find(")", start);

			std::string variable = arg.substr(start + 2, end - (start + 2));
			if(variable.find(" ") != std::string::npos) {
				info::error("Variable cannot contain spaces!");
				return {};
			}

			auto value = get_value(variable);
			if(!std::holds_alternative<std::string>(value)) {
				return {};
			}
			arg.replace(start, end - start + 1, std::get<std::string>(value));
		}
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
