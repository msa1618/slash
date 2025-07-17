#ifndef AMBER_PARSER_H
#define AMBER_PARSER_H

#include <vector>
#include <string>

using Args = std::vector<std::string>;

Args parse_arguments(const std::string& command);
std::vector<Args> parse_pipe_commands(const std::string& command);

#endif // AMBER_PARSER_H
