#ifndef AMBER_CD_H
#define AMBER_CD_H

#include <string>
#include <variant>
#include <vector>

int create_alias(std::string name, std::string dirpath);
std::variant<std::string, int> get_alias(std::string alias);
int cd(std::vector<std::string> args);

#endif // AMBER_CD_H
