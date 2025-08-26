#ifndef SLASH_VAR_H
#define SLASH_VAR_H

#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include <vector>

struct Variable {
  std::string name;
  std::string value;
};

extern std::vector<Variable> temp_vars;

void list_variables();

void create_temp_var(std::string name, std::string value);
void create_variable(std::string name, std::string value);

std::variant<std::string, int> get_value(std::string name);
int var(std::vector<std::string> args);

#endif // SLASH_VAR_H
