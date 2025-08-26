#ifndef SLASH_ALIAS_H
#define SLASH_ALIAS_H

#include <string>
#include <vector>

struct Alias {
  std::string alias;
  std::string value;
};

extern std::vector<Alias> temp_aliases;
void create_temp_alias(std::string cmd, std::string value);

std::string get_alias(std::string name, bool print = false);
int alias(std::vector<std::string> args);
void create_alias(const std::string& name, const std::string& value);

#endif // SLASH_ALIAS_H
