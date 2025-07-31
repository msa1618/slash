#ifndef SLASH_PROMPT_H
#define SLASH_PROMPT_H

#include <string>
#include <variant>
#include <vector>
#include "../abstractions/json.hpp"

std::variant<std::string, int> read_input();
std::string print_prompt(nlohmann::json& j);


#endif // SLASH_PROMPT_H
