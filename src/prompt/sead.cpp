#include <prompt/sead.h>
#include <terminal.h>
#include <iofuncs.h>
#include <prompt/sead.h>

std::string Sead::readln() {
  Terminal::enableRawMode();

  io::error(prompt);

  char c;
  int char_pos = 0;
  std::string buffer;

  while(true) {
    if(read(STDIN_FILENO, &c, 1) != 1) continue;

    if(c == '\n' || c == '\r') {
      io::print("\n");
      break;
    }
    if(c == '\b' || c == 127) {
      if(char_pos > 0 && !buffer.empty()) { 
        buffer.erase(buffer.begin() + (char_pos - 1));
        char_pos--;

        io::print("\r\x1b[2K");
        io::print(prompt + buffer);
      }
    }

    if(isprint(c)) {
      buffer.insert(buffer.begin() + char_pos, c);
      char_pos++;
      io::print("\r\x1b[2K");
      io::print(prompt + buffer);
    }
  }

  Terminal::disableRawMode();
  return buffer;
}
