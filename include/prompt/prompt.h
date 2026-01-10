#ifndef SLASH_PROMPT_H
#define SLASH_PROMPT_H

#include <string>
#include <sstream>
#include "../global.h"

namespace Prompt {

  template <int n>
  std::string get_ps() {
    static_assert(n >= 1 && n <= 4, "n must be between 1 and 4");

    std::string name = "PS" + std::to_string(n);

    if(Global::interpreter.get_scope().has(name)) return Global::interpreter.get_scope().get(name);

    // defaults
    std::vector<std::string> ps_list = {
      R"([green]\u@\h[reset]:[blue]\w[reset] \$ )",
      "[yellow].. [reset]",
      "#? ",
      "+ "
    };
    return ps_list[n - 1];
  }

  std::string parse_ps(std::string value);
}

#endif