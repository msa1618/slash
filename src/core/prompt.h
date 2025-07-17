#ifndef SLASH_PROMPT_H
#define SLASH_PROMPT_H

#include <string>
#include <variant>
#include <vector>

std::variant<std::string, int> read_input();
std::string print_prompt();

#endif // SLASH_PROMPT_H
