#ifndef SLASH_PARSER_H
#define SLASH_PARSER_H

#include <vector>
#include <string>

using Args = std::vector<std::string>;

Args parse_arguments(std::string command);
std::vector<Args> parse_pipe_commands(const std::string& command);

#endif // SLASH_PARSER_H
