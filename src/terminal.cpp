#include <terminal.h>

namespace Terminal {
  struct termios orig_termios;

  void disableRawMode() {
      tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
  }

  void enableRawMode() {
      tcgetattr(STDIN_FILENO, &orig_termios);

      struct termios raw = orig_termios;
      raw.c_lflag &= ~(ECHO | ICANON);
      raw.c_cflag |= (CS8);

      tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }
}