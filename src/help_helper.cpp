#include "help_helper.h"
#include "cmd_highlighter.h"
#include "abstractions/definitions.h"
#include "core/prompt.h"
#include <sstream>

std::string osc_link(std::string url, std::string text) {
    return "\033]8;;" + url + "\033\\" + text + "\033]8;;\033\\";
}

std::string get_helpmsg(HelpMessage msg) {
  std::stringstream ss;
  ss << green << "\nFunction\n" << reset;
  ss << "  " << msg.function << "\n\n";

  ss << green << "Usage\n" << reset;
  for(auto& usage : msg.usages){
    ss << "  " << highl(usage) << "\n";
  }

  ss << "\n";

  if(!msg.flags.empty()) {
    ss << green << "Flags\n" << reset;

    for(auto& flag : msg.flags) {
      ss << "  ";
      if(!flag.short_form.empty()) ss << highl(flag.short_form);
      if(!flag.short_form.empty() && !flag.long_form.empty()) ss << " | ";
      if(!flag.long_form.empty()) {
        if(flag.short_form.empty()) ss << "     ";
        ss << highl(flag.long_form);
      }
      ss << ": " << reset;
      ss << flag.desc << "\n";
    }
  }

  ss << "\n";

  if(!msg.examples.empty()) {
    ss << green << "Examples\n" << reset;
    for(auto& ex : msg.examples) {
      ss << "  " << highl(ex.cmd) << ": " << ex.desc << "\n";
    }
  }

  ss << "\n";

  if(!msg.custom.empty()) {
    ss << msg.custom;
  }

  if(!msg.optional_tip.empty()) {
    ss << green << "Tip: " << reset << msg.optional_tip;
  }

  return ss.str();
}