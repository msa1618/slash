#include "core/execution.h"
#include "core/prompt.h"
#include "core/startup.h"
#include "core/parser.h"

#include <signal.h>

#include "abstractions/iofuncs.h"
#include "abstractions/info.h"

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
		if(input == "exit") exit(0);

		std::vector<std::string> args = parse_arguments(input);

		if(io::vecContains(args, "|")) {
			disable_raw_mode();
			signal(SIGINT, SIG_DFL);
			std::vector<std::vector<std::string>> commands;
			for(auto& cmd : args) {
				cmd = io::trim(cmd);
				std::vector<std::string> args = parse_arguments(cmd);
				commands.push_back(args);
			}
			pipe_execute(commands);
		} else {
			disable_raw_mode();
			signal(SIGINT, SIG_DFL);
			execute(parse_arguments(input), input, true);
		}
		enable_raw_mode();

		signal(SIGINT, SIG_IGN);
	}
}