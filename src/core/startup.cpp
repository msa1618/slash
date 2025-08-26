#include "startup.h"
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "execution.h"
#include "parser.h"
#include "cnf.h"

void enable_canonical_mode() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // start from current state to avoid messing unrelated flags

    t.c_lflag |= ICANON | ECHO | ISIG; // enable canonical input, echo, signals
    t.c_iflag |= ICRNL;                // translate CR to NL
    t.c_oflag |= OPOST;                // enable output processing

    t.c_cc[VMIN] = 1;                  // minimum chars for read
    t.c_cc[VTIME] = 0;                 // no timeout

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
}

void enable_raw_mode() {
    struct termios orig;

    // Get current terminal attributes and save to orig
    if (tcgetattr(STDIN_FILENO, &orig) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    termios raw = orig;

    // Input flags - disable canonical mode, and other input processing
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP);

    // Control flags - set 8-bit chars
    raw.c_cflag |= (CS8);

    // Local flags - disable echo, canonical mode, extended functions and signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Control chars - read returns as soon as 1 byte is available, no timeout
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}


///////////////////////////////////////////////////////

std::string interpret_escapes(const std::string& input) {
  std::string result;
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '\\' && i + 1 < input.size()) {
      char next = input[i + 1];
      if (next == 'n') {
        result += '\n';
        ++i;
      } else if (next == 'x' && i + 3 < input.size()) {
        std::string hex_str = input.substr(i + 2, 2);
        char ch = (char)std::stoi(hex_str, nullptr, 16);
        result += ch;
        i += 3;
      } else {
        result += next;
        ++i;
      }
    } else {
      result += input[i];
    }
  }
  return result;
}

void execute_startup_commands() {
  fill_commands();
  auto startup_commands = io::read_file(slash_dir + "/.slashrc");

  if(std::holds_alternative<int>(startup_commands)) {
    info::error(strerror(errno), errno, ".slashrc");
    return;
  }

  std::string new_s = (getenv("LD_LIBRARY_PATH") != nullptr ? getenv("LD_LIBRARY_PATH") : "") + std::string(":") + std::string(getenv("HOME")) + "/.slash/slash-utils";
  setenv("LD_LIBRARY_PATH", new_s.c_str(), 1);

  std::vector<std::string> lines = io::split(std::get<std::string>(startup_commands), "\n");
  for(auto& command : lines) {
    command = interpret_escapes(command);

    if(command.empty()) continue;
    std::vector<std::string> tokens = parse_arguments(command);
    exec(tokens, command);
  }
}

