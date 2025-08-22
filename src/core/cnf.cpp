#include "cnf.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <unistd.h>
#include <algorithm>
#include <dirent.h>
#include <limits>
#include <sstream>
#include <vector>
#include <cstdlib>

static std::vector<std::string> commands;

int d_levenshtein(const std::string &s1, const std::string &s2) {
    size_t len1 = s1.size();
    size_t len2 = s2.size();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; i++) d[i][0] = i;
    for (size_t j = 0; j <= len2; j++) d[0][j] = j;

    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            d[i][j] = std::min({ d[i-1][j] + 1,     // deletion
                                 d[i][j-1] + 1,     // insertion
                                 d[i-1][j-1] + cost // substitution
                               });
            if (i > 1 && j > 1 && s1[i-1]==s2[j-2] && s1[i-2]==s2[j-1]) {
                d[i][j] = std::min(d[i][j], d[i-2][j-2] + cost); // transposition
            }
        }
    }
    return d[len1][len2];
}

void fill_commands() {
    char* path_env = getenv("PATH");
    if(!path_env) return;

    std::vector<std::string> bindirs = io::split(path_env, ":");
    char* home_env = getenv("HOME");
    if(home_env)
        bindirs.push_back(std::string(home_env) + "/.slash/slash-utils");

    for(auto& dir : bindirs) {
        DIR* d = opendir(dir.c_str());
        if(!d) continue;

        struct dirent* entry;
        while((entry = readdir(d)) != nullptr) {
            std::string name = entry->d_name;
            if(name == "." || name == "..") continue;
            commands.push_back(name);
        }
        closedir(d);
    }
}

std::string cnf(std::string cmd) {
    std::stringstream ss;
    ss << red << "[Error] " << reset << cmd << ": Command not found\n";

    std::string best_match;
    int min_dist = std::numeric_limits<int>::max();

    for(const auto& command : commands) {
        int distance = d_levenshtein(cmd, command);
        if(distance < min_dist) {
            min_dist = distance;
            best_match = command;
        }
    }

    if(min_dist > 0 && min_dist < 3) { // threshold for suggestion
        ss << "Did you mean " << cyan << "\"" << best_match << "\"" << reset << "?\n";
    }

    return ss.str();
}
