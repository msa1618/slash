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
        buffer.pop_back();
        char_pos--;
        io::print("\b \b");
      }
    }

    if(isprint(c)) {
      buffer += c;
      char_pos++;
      io::print(c);
    }
  }

  Terminal::disableRawMode();
  return buffer;
}
