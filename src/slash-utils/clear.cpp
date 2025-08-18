#include <vector>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../help_helper.h"

class Clear {
  private:
    int clear(bool dont_clear_scrollback, bool keep_cursor_pos) {
      if(dont_clear_scrollback && keep_cursor_pos) {
        io::print("\x1b[2J");
        return 0;
      }

      if(dont_clear_scrollback) {
        io::print("\x1b[2J\x1b[H");
        return 0;
      } 
      
      if(keep_cursor_pos) {
        // Might not work everywhere
        io::print("\x1b[3J");
        return 0;
      }

      io::print("\x1b[3J\x1b[H\x1b[0J");
      return 0;
    }

  public:
    Clear() {}

    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        return clear(false, false);
      }

      std::vector<std::string> valid_args = {
        "-k", "--keep-cursor-pos",
        "-s", "--keep-scrollback",
        "-h", "--help"
      };

      bool keep_pos = false;
      bool keep_scr = false;

      for(auto& arg : args) {
        if(arg.starts_with("-") && !io::vecContains(valid_args, arg)) {
          info::error("Invalid argument \"" + arg + "\"");
          return -1;
        }

        if(arg == "-h" || arg == "--help") {
          io::print(get_helpmsg({
            "Clears terminal",
            {
              "clear",
              "clear [option]"
            },
            {
              {"-s", "--keep-scrollback", "Do not clear the scrollback buffer"},
              {"-k", "--keep-cursor-pos", "Clear without putting cursor at home (Ln 1, Col 1) " + yellow + "(Might not work everywhere)" + reset},
              {"-h", "--help", "Show this help message"}
            },
            {
              {"clear", "Clears entire screen"},
              {"clear -s", "Clear without clearing scrollback"},
              {"clear -s -k", "Preserve the scrollback buffer and the cursor position"}
            },
            "",
            ""
          }));
          return 0;
        }

        if(arg == "-s" || arg == "--keep-scrollback") keep_scr = true;
        if(arg == "-k" || arg == "--keep-cursor-pos") keep_pos = true;
      }

      return clear(keep_scr, keep_pos);
    }
};

int main(int argc, char* argv[]) {
  Clear clear;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  clear.exec(args);
  return 0;
}