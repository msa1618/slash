#include "tui.h"
#include "../abstractions/iofuncs.h"

#include <unordered_map>
#include <sstream>

static struct termios orig_tty;

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_tty);  // Save original
    struct termios tty = orig_tty;
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_tty);  // Restore original
}

void Tui::switch_to_alternate() {
  io::print("\x1b[?1049h");
}

void Tui::switch_to_normal() {  
    io::print("\x1b[?1049l");
}

void Tui::clear() {
    io::print("\x1b[2J\x1b[H");
}

void Tui::turn_cursor(bool b) {
  if (b) io::print("\x1b[?25h");
  else   io::print("\x1b[?25l");
}

void Tui::cleanup_and_exit(int code) {
  disable_raw_mode();
  clear();
  switch_to_normal();
  turn_cursor(ON);
  _exit(code);
}

void Tui::move_cursor_to_last() {
    int last_row = Tui::get_terminal_height();
    io::print("\x1b[" + std::to_string(last_row) + ";1H");  
}

void Tui::move_cursor_to_first() {
    // Move cursor to row 1, column 1 (top-left corner)
    io::print("\x1b[1;1H");
}

void Tui::move_cursor_to(int row, int col) {
    io::print("\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H");
}

void Tui::scroll_up() {
    // Scroll up one line (content moves down)
    io::print("\x1b[S");
}

void Tui::scroll_down() {
    // Scroll down one line (content moves up)
    io::print("\x1b[T");
}

void Tui::turn_linewrap(bool b) {
  if(b) io::print("\x1b[?7l");
  else io::print("\x1b[?7h");
}

void Tui::print_in_center_of_terminal(std::string text) {
  int height = get_terminal_height();
  int width = get_terminal_width();

  std::vector<std::string> lines = io::split(text, "\n");

  int max_length = 0;
  for (auto& l : lines) {
    int len = io::strip_ansi(l).size();
    if (len > max_length) max_length = len;
  }

  int startRow = (height - static_cast<int>(lines.size())) / 2;
  int startCol = (width - max_length) / 2;

  for (size_t i = 0; i < lines.size(); ++i) {
    int colOffset = startCol;
    std::stringstream ss;
    ss << "\033[" << (startRow + i + 1) << ";" << (colOffset + 1) << "H" 
              << lines[i];
    io::print(ss.str());
  }
  fflush(stdout);
}

void Tui::print_separator() {
  for(int i = 0; i < Tui::get_terminal_width(); i++) {
     io::print("─");
  } io::print("\n");
}

int Tui::get_terminal_width() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

int Tui::get_terminal_height() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
}

static int tty_fd = -1;

void init_tty() {
  tty_fd = open("/dev/tty", O_RDONLY | O_NONBLOCK);
  if (tty_fd < 0) {
    tty_fd = tty_fd;
  }
}

std::string Tui::get_keypress() {
  if(tty_fd < 0) init_tty();

  char c;
  if(read(tty_fd, &c, 1) != 1) return "";
  
  if(c == '\t') return "Tab";
  if(c == '\r' || c == '\n') return "Enter";
  
  if(c == 27) {
    char seq[2];
    if (read(tty_fd, &seq[0], 1) != 1) return "";
    if (read(tty_fd, &seq[1], 1) != 1) return "";

    switch(seq[1]) {
      case 'A': return "ArrowUp";
      case 'B': return "ArrowDown";
      case 'C': return "ArrowRight";
      case 'D': return "ArrowLeft";
    }
  }

  if(c >= 1 && c <= 26) {
    char ctrl_char = 'A' + (c - 1);
    return "Ctrl+" + std::string(1, ctrl_char);
  }

  if(c == 127 || c == 8) return "Backspace";

  return std::string(1, c);
}


