#ifndef SLASH_TERMINAL_H
#define SLASH_TERMINAL_H

#include <termios.h>
#include <unistd.h>
#include <cstdlib>

namespace Terminal {
  extern struct termios orig_termios;

  void disableRawMode();
  void enableRawMode();
};

#endif