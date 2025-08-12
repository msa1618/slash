#include "../tui/tui.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <sstream>
#include <csignal>
#include <regex>

class Pager {
  private:
  /*
    void print_header(std::string filepath) {
      io::print("\x1b[7m");
      std::string text = filepath.empty() ? "pager" : "pager - " + filepath;
      Tui::move_cursor_to(0, (Tui::get_terminal_width() / 2) - text.length());
      io::print(text);
      io::print(std::string(Tui::get_terminal_width() / 2, ' '));
      io::print(reset);

      for (int i = 0; i < Tui::get_terminal_width(); ++i) {
        io::print("─");
      }
    }
  */

    std::string strip_ansi(const std::string& input) {
      static const std::regex ansi_regex("\x1b\\[[0-9;]*[mGKHF]");
      return std::regex_replace(input, ansi_regex, "");
    }

    void print_footer(std::string filepath, int percentage) {
      int width = Tui::get_terminal_width();

      std::stringstream footer;
      footer << bg_blue << black << " pager " << reset << bg_gray;
      footer << " " << filepath;

      std::string left = footer.str();
      std::string right = "(↑/↓ to scroll) " + std::to_string(percentage) + "% ";

      int space_len = (width - strip_ansi(left).length() - strip_ansi(right).length()) + 4; // 4 because of the extra size they take coz theyre not ascii
      if(space_len < 0) space_len = 0;

      std::string line = left + std::string(space_len, ' ') + right + reset;
  
      Tui::move_cursor_to_last();

      io::print(line);
    }

    int page(std::string text, bool text_isfilepath) {
      std::string ttu; // text to use
      int height = Tui::get_terminal_height() - 1;

      if(text_isfilepath) {
        auto content = io::read_file(text);
        if(!std::holds_alternative<std::string>(content)) {
          info::error(std::string("Failed to read file: ") + strerror(errno));
          return errno;
        }
        ttu = std::get<std::string>(content);
      } else ttu = text;

      Tui::switch_to_alternate();
      enable_raw_mode();
      Tui::clear();
      Tui::turn_cursor(OFF);

      signal(SIGINT, Tui::cleanup_and_exit);
      signal(SIGTERM, Tui::cleanup_and_exit);

      std::vector<std::string> lines = io::split(ttu, "\n");
      int scroll_offset = 0;

      auto redraw = [&](int sig = 0) {
        Tui::clear();
        for (int i = scroll_offset; i < std::min(scroll_offset + height, (int)lines.size()); i++) {
          io::print(lines[i] + "\n");
        }

        std::string first_arg = text_isfilepath ? text : "";

        int percent = (int)(((scroll_offset + height) * 100.0) / lines.size());
        if (percent > 100) percent = 100;
        print_footer(first_arg, percent);
      };

      redraw();

      while(true) {
        std::string key = Tui::get_keypress();

        if(key == "ArrowUp") {
          if(scroll_offset > 0) {
            scroll_offset--;
            redraw();
          }
        }

        if(key == "ArrowDown") {
          if(scroll_offset + height < (int)lines.size()) {
            scroll_offset++;
            redraw();
          }
        }


        if(key == "q" || key == "Q") Tui::cleanup_and_exit(0);
      }
    }

    public:
    Pager() {}

    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        io::print(R"(pager: paginate long text i.e. use entire terminal to display text
usage: pager <filepath>
       pager -t <text>
       cmd | pager -t
       
flags:
  -t | --text: the incoming argument is text
)");
          return 0;
      }

      std::string a;
      bool is_filepath = true;

      for(auto& arg : args) {
        if(!io::vecContains(args, arg) && arg.starts_with("-")) {
          info::error("Invalid argument \"" + arg + "\"");
          return -1;
        }

        if(arg == "-t" || arg == "--text") is_filepath = false;
        if(!arg.starts_with("-")) a = arg;
      }

      if(!is_filepath && a.empty()) {
        std::stringstream ss; ss << std::cin.rdbuf();
        a = ss.str();
      }

      return page(a, is_filepath);
    }
};

int main(int argc, char* argv[]) {
  Pager pager;
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return pager.exec(args);
}