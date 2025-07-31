#ifndef SLASH_CD_H
#define SLASH_CD_H

#include <string>
#include <variant>
#include <vector>

int create_alias(std::string name, std::string dirpath);
std::variant<std::string, int> get_cd_alias(std::string alias);
int cd(std::vector<std::string> args);

#endif // SLASH_CD_H
