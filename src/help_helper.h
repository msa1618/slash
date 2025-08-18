#ifndef SLASH_HELP_HELPER_H
#define SLASH_HELP_HELPER_H

#include <string>
#include <vector>

struct Question {
  std::string ques;
  std::string ans;
};

struct Example {
  std::string cmd;
  std::string desc;
};

struct Flag {
  std::string short_form;
  std::string long_form;
  std::string desc;
};

struct HelpMessage {
  std::string function;
  std::vector<std::string> usages;
  std::vector<Flag> flags;
  std::vector<Example> examples;
  std::string custom;
  std::string optional_tip;
};

std::string osc_link(std::string url, std::string text);
std::string get_helpmsg(HelpMessage msg);

#endif // SLASH_HELP_HELPER_H