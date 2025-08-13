#include "core/execution.h"
#include "core/prompt.h"
#include "core/startup.h"
#include "core/parser.h"
#include "abstractions/json.h"

#include "builtin-cmds/cd.h"

#include <signal.h>

#include "abstractions/iofuncs.h"
#include "abstractions/info.h"
#include <termios.h>

#define EXIT 5000

int exec(std::vector<std::string> args, std::string raw_input, bool save_to_history) {
	raw_input = io::trim(raw_input);
	if(args.empty()) return 0;
	if(args[0] == "exit") {
		enable_canonical_mode();
		return EXIT;
	}

	if(args[0] == "cd") {
		return cd(args);
	}

	bool bg = false;
	bool devnull = false;
	if(raw_input.ends_with("&n")) {
    bg = true;
    devnull = true;
    args.pop_back();
	} else if(raw_input.ends_with("&")) {
			bg = true;
			args.pop_back();
	}

	if(io::split(raw_input, ";").size() > 1) {
		for(auto cmd : io::split(raw_input, ";")) {
			cmd = io::trim(cmd);
			int res = exec(parse_arguments(cmd), cmd, save_to_history);
			if (res == EXIT) return EXIT;
		}
		return 0;
	}

	if(io::split(raw_input, "&&").size() > 1) {
		for(auto cmd : io::split(raw_input, "&&")) {
			cmd = io::trim(cmd);
			int res = execute(parse_arguments(cmd), cmd, save_to_history, bg, devnull);
			if (res == EXIT) return EXIT;
			if (res != 0) break;
		}
		return 0;
	}

	if(io::split(raw_input, "||").size() > 1) {
		for(auto cmd : io::split(raw_input, "||")) {
			cmd = io::trim(cmd);
			int res = execute(parse_arguments(cmd), cmd, save_to_history, bg, devnull);
			if (res == EXIT) return EXIT;
			if (res == 0) break;
		}
		return 0;
	}

	if(io::split(raw_input, "|").size() > 1) {
		std::vector<std::vector<std::string>> commands;
		for(auto& cmd : io::split(raw_input, "|")) {
			cmd = io::trim(cmd);
			commands.push_back(parse_arguments(cmd));
		}
		return pipe_execute(commands);
	}

	if (raw_input.find(">") != std::string::npos || raw_input.find("2>") != std::string::npos) {
		std::string trimmed_input = io::trim(raw_input);

		bool is_stdout = trimmed_input.find("2>") == std::string::npos;
		bool append = trimmed_input.find(">>") != std::string::npos;

		std::string redirect_op = append ? ">>" : (is_stdout ? ">" : "2>");
		std::vector<std::string> parts = io::split(trimmed_input, redirect_op);
		if (parts.size() != 2) {
			info::error("Invalid redirection syntax", 1);
			return -1;
		}

		std::string command_part = io::trim(parts[0]);
		std::string path_part = io::trim(parts[1]);

		std::vector<std::string> new_args = parse_arguments(command_part);

		return redirect(new_args, command_part, save_to_history, path_part, append, is_stdout);
	}

	return execute(args, raw_input, save_to_history, bg, devnull);
}


int main(int argc, char* argv[]) {
    signal(SIGINT, SIG_IGN);

		std::string HOME = getenv("HOME");
    auto cnt = get_json(HOME + "/.slash/config/prompts/default.json");

    if(argc > 1) {
        std::vector<std::string> args;
        args.reserve(argc - 1);

        for(int i = 1; i < argc; i++) {
            args.emplace_back(argv[i]);
        }

        auto original_args = args;
        args = parse_arguments(io::join(args, " "));

        exec(args, io::join(original_args, " "), true);
        return 0;
    }

    execute_startup_commands();
    enable_raw_mode();

    while(true) {
        std::string input = print_prompt(cnt);
        if(input.empty() || input.starts_with("#")) continue;

        std::vector<std::string> args = parse_arguments(input);

        if(exec(args, input, true) == EXIT) break;
    }
    return 0;
}
