#ifndef AMBER_ALIAS_H
#define AMBER_ALIAS_H

#include <string>
#include <vector>

std::string get_alias(std::string name, bool print = false);
int alias(std::vector<std::string> args);
void create_alias(const std::string& name, const std::string& value);

#endif // AMBER_ALIAS_H
