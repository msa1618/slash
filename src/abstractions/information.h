#include <iostream>
#include "iofuncs.h"

namespace info {
    void command_too_lengthy(std::vector<std::string> args) {
        decltype(args) l_args;
        for(auto& arg : args) { if(arg.starts_with("--")) l_args.emplace_back(arg); }
        
    }
}