#ifndef SLASH_SEAD_H
#define SLASH_SEAD_H

#include <unistd.h>
#include <string>

class Sead {
  private:
    std::string prompt;
    int n;

  public:
    Sead(std::string prompt) : prompt(prompt) {}

    std::string readln();
    int readint();
};

#endif