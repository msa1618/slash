#include "core/execution.h"
#include "core/prompt.h"
#include "core/startup.h"
#include "core/parser.h"
#include "abstractions/json.h"

#include "builtin-cmds/cd.h"

#include <signal.h>

#include "abstractions/iofuncs.h"
#include "abstractions/info.h"

void exec(std::vector<std::string> args, std::string raw_input, bool save_to_history) {
	if(args[0] == "exit") {
		disable_raw_mode();
		fflush(stdout);
		exit(0);
	}

	if(args[0] == "cd") {
		info::debug("Running cd..");
		cd(args);
		return;
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

	if (raw_input.find(">") != std::string::npos || raw_input.find("2>") != std::string::npos) {
    std::string trimmed_input = io::trim(raw_input);

    bool is_stdout = trimmed_input.find("2>") == std::string::npos;
    bool append = trimmed_input.find(">>") != std::string::npos;

    // Split on '>', '>>', or '2>' to extract command and file path
    std::string redirect_op = append ? ">>" : (is_stdout ? ">" : "2>");
    std::vector<std::string> parts = io::split(trimmed_input, redirect_op);
    if (parts.size() != 2) {
        info::error("Invalid redirection syntax", 1);
        return;
    }

    std::string command_part = io::trim(parts[0]);
    std::string path_part = io::trim(parts[1]);

    std::vector<std::string> new_args = parse_arguments(command_part);

    redirect(new_args, command_part, save_to_history, path_part, append, is_stdout);
    return;
}

  execute(args, raw_input, save_to_history);
}

int main(int argc, char* argv[]) {
	signal(SIGINT, SIG_IGN);

	auto cnt = get_json("/home/aetheros/.slash/config/prompts/default.json");

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
	
	execute_startup_commands();
	enable_raw_mode();

	while(true) {
		std::string input = print_prompt(cnt);
		if(input.empty() || input.starts_with("#") || input.starts_with("//")) continue;

		std::vector<std::string> args = parse_arguments(input);

		exec(args, input, true);
		enable_raw_mode();

		signal(SIGINT, SIG_IGN);
	}

	disable_raw_mode();
	return 0;
}