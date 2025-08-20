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

int main(int argc, char* argv[]) {
    signal(SIGINT,  SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    std::string HOME = getenv("HOME");

    auto settings = get_json(HOME + "/.slash/config/settings.json");
    auto settings_path = get_string(settings, "pathOfPromptTheme");
    if(!settings_path) {
        info::error("pathOfPromptTheme doesn't exist.");
        return -1;
    }

    auto cnt = get_json(*settings_path);

    if(argc > 1) {
        std::vector<std::string> args;
        args.reserve(argc - 1);

        for(int i = 1; i < argc; i++) {
            args.emplace_back(argv[i]);
        }

        auto original_args = args;
        args = parse_arguments(io::join(args, " "));
        save_to_history(args, io::join(args, " "));

        exec(args, io::join(original_args, " "));
        return 0;
    }

    execute_startup_commands();
    enable_raw_mode();

    while(true) {
        std::string input = print_prompt(cnt);
        if(input.empty() || input.starts_with("#")) continue;

        std::vector<std::string> args = parse_arguments(input);
        if(!args.empty()) save_to_history(args, input);

        exec(args, input);

        // re-enable
        enable_raw_mode();
    }
    return 0;
}
