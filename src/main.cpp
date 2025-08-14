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

int exec(std::vector<std::string> args, std::string raw_input, bool save_to_his) {
    raw_input = io::trim(raw_input);
    if (args.empty()) return 0;

    if (args[0] == "exit") {
        enable_canonical_mode();
        return EXIT;
    }

    if (args[0] == "cd") {
        return cd(args);
    }

		bool bg = false;
		if (raw_input.ends_with("&")) {
        bg = true;
        args.pop_back();
    }

    // Handle redirection
    RedirectInfo rinfo;
		rinfo.enabled = false;
    if (io::vecContains(args, ">") || io::vecContains(args, ">>") || io::vecContains(args, "2>") || io::vecContains(args, "2>>")) {
        bool stdout_ = !io::vecContains(args, "2>") && !io::vecContains(args, "2>>");
        rinfo.stdout_ = stdout_;
        rinfo.append = io::vecContains(args, ">>") && io::vecContains(args, "2>>");
        rinfo.enabled = true;

        std::string redirect_op = rinfo.append ? (stdout_ ? ">>" : "2>>") : (stdout_ ? ">" : "2>");
        std::vector<std::string> parts = io::split(raw_input, redirect_op);
        if (parts.size() != 2) {
            info::error("Invalid redirection syntax", 1);
            return -1;
        }

        std::string command_part = io::trim(parts[0]);
        rinfo.filepath = io::trim(parts[1]);
        std::vector<std::string> new_args = parse_arguments(command_part);

        return execute(new_args, command_part, save_to_his, bg, rinfo);
    }

    // Handle multiple commands separated by ";"
    if (io::split(raw_input, ";").size() > 1) {
				save_to_history(parse_arguments(raw_input), raw_input);
        for (auto cmd : io::split(raw_input, ";")) {
            cmd = io::trim(cmd);
            int res = exec(parse_arguments(cmd), cmd, false);
            if (res == EXIT) return EXIT;
        }
        return 0;
    }

    // Handle "&&"
    if (io::split(raw_input, "&&").size() > 1) {
				save_to_history(parse_arguments(raw_input), raw_input);
        for (auto cmd : io::split(raw_input, "&&")) {
            cmd = io::trim(cmd);
            int res = exec(parse_arguments(cmd), cmd, false);
            if (res == EXIT) return EXIT;
            if (res != 0) break;
        }
        return 0;
    }

    // Handle "||"
    if (io::split(raw_input, "||").size() > 1) {
				save_to_history(parse_arguments(raw_input), raw_input);
        for (auto cmd : io::split(raw_input, "||")) {
            cmd = io::trim(cmd);
            int res = exec(parse_arguments(cmd), cmd, false);
            if (res == EXIT) return EXIT;
            if (res == 0) break;
        }
        return 0;
    }

    // Handle pipes
    if (io::split(raw_input, "|").size() > 1) {
        std::vector<std::vector<std::string>> commands;
        for (auto& cmd : io::split(raw_input, "|")) {
            cmd = io::trim(cmd);
            commands.push_back(parse_arguments(cmd));
        }
        return pipe_execute(commands);
    }

    return execute(args, raw_input, save_to_his, bg, rinfo);
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
				// re-enable
				enable_raw_mode();
    }
    return 0;
}
