#include "parser.h"
#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"

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

	if (!buffer.empty()) {
		args.push_back(buffer);
	}

	return args;
}

std::vector<Args> parse_pipe_commands(const std::string& command) {
	std::vector<Args> result;
	Args pipe_cmds = io::split(command, '|');
	for (auto& cmd : pipe_cmds) {
		cmd = io::trim(cmd);
		result.push_back(parse_arguments(cmd));
	}
	return result;
}
