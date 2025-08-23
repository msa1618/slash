#include "json.h"
#include "iofuncs.h"
#include "info.h"

#include <string>
#include <cstring> // for strerror

#include "json.hpp"

nlohmann::json get_json(const std::string& path) {
    auto content = io::read_file(path);
    if (!std::holds_alternative<std::string>(content)) {
        std::string error = std::string("Failed to open \"") + path + "\": " + strerror(errno);
        info::error(error, errno);
        return {};
    }

    try {
        auto data = std::get<std::string>(content);
        return nlohmann::json::parse(data);
    } catch (const std::bad_variant_access& e) {
        info::error("Invalid file content type", -1);
        return {};
    } catch (const nlohmann::json::parse_error& e) {
        std::string error = std::string("JSON parse error in \"") + path + "\": " + e.what();
        info::error(error, -1);
        return {};
    }
}

std::optional<bool> get_bool(const nlohmann::json& json, const std::string& property, const std::string& object) {
    if (object.empty()) {
        if (json.contains(property) && json[property].is_boolean()) {
            return json[property].get<bool>();
        }
        return std::nullopt;
    } else {
        if (!json.contains(object) || !json[object].contains(property) || !json[object][property].is_boolean()) {
            return std::nullopt;
        }
        return json[object][property].get<bool>();
    }
}

std::optional<std::string> get_string(const nlohmann::json& json, const std::string& property, const std::string& object) {
    if (object.empty()) {
        if (json.contains(property) && json[property].is_string()) {
            return json[property].get<std::string>();
        }
        return std::nullopt;
    } else {
        if (!json.contains(object) || !json[object].contains(property) || !json[object][property].is_string()) {
            return std::nullopt;
        }
        return json[object][property].get<std::string>();
    }
}


std::optional<int> get_int(const nlohmann::json& json, const std::string& property, const std::string& object) {
     if (object.empty()) {
        if (json.contains(property) && json[property].is_number()) {
            return json[property].get<int>();
        }
        return std::nullopt;
    } else {
        if (!json.contains(object) || !json[object].contains(property) || !json[object][property].is_number()) {
            return std::nullopt;
        }
        return json[object][property].get<int>();
    }
}
std::optional<std::array<int, 3>> get_int_array3(const nlohmann::json& json, const std::string& property, const std::string& object) {
    nlohmann::json target;

    if (object.empty()) {
        if (!json.contains(property) || !json[property].is_array()) return std::nullopt;
        target = json[property];
    } else {
        if (!json.contains(object) || !json[object].contains(property) || !json[object][property].is_array()) {
            return std::nullopt;
        }
        target = json[object][property];
    }

    if (target.size() != 3) return std::nullopt;

    std::array<int, 3> result;
    for (int i = 0; i < 3; ++i) {
        if (!target[i].is_number_integer()) return std::nullopt;
        result[i] = target[i].get<int>();
    }

    return result;
}
