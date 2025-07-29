#include "core/execution.h"
#include "core/prompt.h"
#include "core/startup.h"
#include "core/parser.h"

#include <signal.h>

#include "abstractions/iofuncs.h"
#include "abstractions/info.h"

void exec(std::vector<std::string> args, std::string raw_input, bool save_to_history) {
	if(args[0] == "exit") {
		disable_raw_mode();
		fflush(stdout);
		exit(0);
	}

	std::vector<std::string> commands;

	// Priority:
	// 1. Semicolons: They separate commands, and execute them regardless of the first command's success or failure.
	// 2. AND (&&): Executes the next command only if the previous command succeeded (exit code 0).
	// 3. OR (||): Executes the next command only if the previous command failed
	// 4. Pipes
	// 5. Normal command execution

	if(io::split(raw_input, ";").size() > 1) {
		for(auto cmd : io::split(raw_input, ";")) {
			cmd = io::trim(cmd);
			exec(parse_arguments(cmd), cmd, save_to_history);
		}
		return;
	}

	if(io::split(raw_input, "&&").size() > 1) {
		for(auto cmd : io::split(raw_input, "&&")) {
			cmd = io::trim(cmd);
			if(execute(parse_arguments(cmd), cmd, save_to_history) == 0) {
				continue;
			} else break;
		}
		return;
	}

	if(io::split(raw_input, "||").size() > 1) {
		for(auto cmd : io::split(raw_input, "||")) {
			cmd = io::trim(cmd);
			if(execute(parse_arguments(cmd), cmd, save_to_history) != 0) {
				continue;
			} else break;
		}
	}

	if(io::split(raw_input, "|").size() > 1) {
		std::vector<std::vector<std::string>> commands;
		for(auto& cmd : io::split(raw_input, "|")) {
			cmd = io::trim(cmd);
			commands.push_back(parse_arguments(cmd));
		}
		pipe_execute(commands);
		return;
	}

  execute(args, raw_input, save_to_history);
}

int main(int argc, char* argv[]) {
	signal(SIGINT, SIG_IGN);

	if(argc > 1) {
		std::vector<std::string> args;
		args.reserve(argc - 1);

		for(int i = 1; i < argc; i++) {
			args.emplace_back(argv[i]);
		}

		auto original_args = args;
		args = parse_arguments(io::join(args, " "));

		execute(args, io::join(original_args, " "), true);
		return 0;
	}

	enable_raw_mode();
	execute_startup_commands();

	while(true) {
		std::string input = print_prompt();
		if(input.empty()) continue;

		std::vector<std::string> args = parse_arguments(input);

		exec(args, input, true);
		enable_raw_mode();

		signal(SIGINT, SIG_IGN);
	}

	disable_raw_mode();
	return 0;
}