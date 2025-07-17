#ifndef SLASH_EXECUTION_H
#define SLASH_EXECUTION_H

#include <string>
#include <vector>

int pipe_execute(std::vector<std::vector<std::string>> commands);
int execute(std::vector<std::string> args, bool save_to_history);

#endif // SLASH_EXECUTION_H
