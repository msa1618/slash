#include "core/execution.h"
#include "core/prompt.h"
#include "core/startup.h"
#include "core/parser.h"

#include "abstractions/iofuncs.h"
#include "abstractions/info.h"

int main(int argc, char* argv[]) {
	enable_raw_mode();
	execute_startup_commands();

	while(true) {
		std::string input = print_prompt();
		if(input.empty()) continue;
		if(input == "exit") exit(0);

		std::vector<std::string> args = parse_arguments(input);

		if(io::vecContains(args, "|")) {
			disable_raw_mode();
			std::vector<std::vector<std::string>> commands;
			for(auto& cmd : args) {
				cmd = io::trim(cmd);
				std::vector<std::string> args = parse_arguments(cmd);
				commands.push_back(args);
			}
			pipe_execute(commands);
		} else {
			disable_raw_mode();
			execute(parse_arguments(input), true);
		}
		enable_raw_mode();
	}
}