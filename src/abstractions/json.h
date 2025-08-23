#ifndef SLASH_JSON_H
#define SLASH_JSON_H

#include "json.hpp"
#include <optional>
#include <array>
#include <string>

nlohmann::json get_json(const std::string& path);

std::optional<bool> get_bool(const nlohmann::json& json, const std::string& property, const std::string& object = "");
std::optional<std::string> get_string(const nlohmann::json& json, const std::string& property, const std::string& object = "");
std::optional<int> get_int(const nlohmann::json& json, const std::string& property, const std::string& object);

std::optional<std::array<int, 3>> get_int_array3(const nlohmann::json& json, const std::string& property, const std::string& object = "");


#endif // SLASH_JSON_H