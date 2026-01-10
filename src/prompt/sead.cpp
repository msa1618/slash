#include <prompt/sead.h>
#include <terminal.h>
#include <iofuncs.h>
#include <prompt/sead.h>
#include <history_manager.h>

void Sead::redraw() {
  int times = io::strip_ansi(prompt).size() + char_pos;

  io::print("\r\x1b[2K");
  io::print("%s", (prompt + buffer).c_str());

  if(times > 0) { 
    io::print("\r");
    io::print("\x1b[%dC", times);
  }
}

std::string Sead::readln() {
  Terminal::enableRawMode();

  io::error(prompt.c_str());

  HistoryManager history;

  char c;
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
        io::print("%s", (prompt + buffer).c_str());
      }

      redraw();
    }
    if(c == '\x1b') {
      std::string sequence;
      for(int i = 0; i < 10; i++) {
        char c;
        if(read(STDIN_FILENO, &c, 1) != 1) break;
        if(isalpha(c)) {
          sequence += c;
          break; // end of sequence
        }

        sequence += c;
      }

      if(sequence == "[A") {
        buffer = history.from_last(history_index++);
        char_pos = buffer.size();
        redraw();
      }
      if(sequence == "[B") {
        buffer = history.from_last(history_index--);
        char_pos = buffer.size();
        redraw();
      }
      if(sequence == "[C" && char_pos < buffer.size()) {
        char_pos++;
        redraw();
      }
      if(sequence == "[D" && char_pos > 0) {
        char_pos--;
        redraw();
      }
    }

    if(isprint(c)) {
      buffer.insert(buffer.begin() + char_pos, c);
      char_pos++;
      redraw();
    }
  }

  history.add(buffer);

  Terminal::disableRawMode();
  return buffer;
}
