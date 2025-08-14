#ifndef SLASH_EXECUTION_H
#define SLASH_EXECUTION_H

#include <string>
#include <vector>

struct RedirectInfo {
    bool enabled;
    bool stdout_;
		bool append;
    std::string filepath;
};

int pipe_execute(std::vector<std::vector<std::string>> commands);
int execute(std::vector<std::string> parsed_args, std::string input, bool save_to_history, bool bg, RedirectInfo rinfo);
int save_to_history(std::vector<std::string> parsed_arg, std::string input);

#endif // SLASH_EXECUTION_H
