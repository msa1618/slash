#ifndef SLASH_SEAD_H
#define SLASH_SEAD_H

#include <unistd.h>
#include <string>

class Sead {
  private:
    std::string prompt;
    int n{};

    std::string buffer;
    int char_pos = 0;
    int history_index{};

    void redraw();

  public:
    Sead(std::string prompt) : prompt(prompt) {}

    std::string readln();
    int readint();
};

#endif