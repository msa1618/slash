#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <vector>
#include <boost/regex.hpp>
#include <sys/ioctl.h>
#include <csignal>
#include <thread>
#include <termios.h>

struct termios orig_tty;

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

class Md {
  private:
    static void switch_to_alternate() {
      io::print("\x1b[?1049h");
      // Hide cursor
      io::print("\x1b[?25l");
    }

    static void switch_to_normal() {
      // Show cursor again
      io::print("\x1b[?25h");
      io::print("\x1b[?1049l");
    }

    static void clear() {
      io::print("\x1b[2J\x1b[H");
    }

    int get_terminal_width() {
      struct winsize w;
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      return w.ws_col;
    }

    int get_terminal_height() {
      struct winsize w;
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      return w.ws_row;
    }

    void draw_statusbar(int current_line, std::vector<std::string> lines) {
      io::print("\x1b[" + std::to_string(get_terminal_height()) + ";1H");
      std::stringstream ss;
      ss << bg_green << black << " md " << reset << bg_gray << " q to quit ";


      int percent = lines.size() < get_terminal_height() ? 100 : static_cast<int>((static_cast<double>(current_line) / (int)lines.size()) * 100);
      int width = get_terminal_width();

      if (lines.size() == 0) {
        int space_length = width - 4 - 8 - 11; // 4 for " md ", 8 for "<empty> "
        ss << std::string(space_length, ' ') << yellow << "<empty> " << reset;
      } else {
        std::string percent_str = std::to_string(percent) + "%";
        int padding = width - 4 - 11 - static_cast<int>(percent_str.size()) - 1; // 1 for the single trailing space
        ss << std::string(padding, ' ') << percent_str << ' ';
      }

      ss << reset;
      io::print(ss.str());
    }



    static void sig_handler(int sig) {
      clear();
      switch_to_normal();
      disable_raw_mode();
      _exit(sig);
    }

    int render(std::string text, bool text_isfilepath) {
      std::string ttu; // text to use
      if (text_isfilepath) {
        auto content = io::read_file(text);
        if (!std::holds_alternative<std::string>(content)) {
          info::error(std::string("Failed to read file: ") + strerror(errno), errno);
          return errno;
        }
        ttu = std::get<std::string>(content);
      } else ttu = text;

      signal(SIGINT, sig_handler);
      signal(SIGTERM, sig_handler);

      switch_to_alternate();
      enable_raw_mode();

      const std::string osc8_start = "\x1b]8;;";
      const std::string osc8_end   = "\x1b]8;;\x1b\\";

      // === Regex patterns ===
      boost::regex inline_code(R"(`([^`\n]+)`)");
      boost::regex blockquotes(R"(^>(.*))", boost::regex_constants::perl);
      boost::regex separators(R"(^(-|=|_){3,})", boost::regex_constants::perl);
      boost::regex underl(R"(__([^_]+)__)");
      boost::regex links(R"(\[([^\]]+)\]\(([^\)]+)\))");
      boost::regex imgs(R"(!\[(.*?)\]\((.*?)\))");
      boost::regex hyperlinks(R"(<([^\n>]+)>)");
      boost::regex striketh(R"(\~\~([^\n\~]+)\~\~)");
      boost::regex italic(R"((?<!\*)\*(?!\*)([^*\n]+)(?<!\*)\*(?!\*))");
      boost::regex bld(R"(\*{2}([^\n*]+)\*{2})");
      boost::regex highl(R"(==([^\n=]+)==)");
      boost::regex nums_list(R"(^\d+\.)", boost::regex_constants::perl);
      boost::regex unordered(R"(^-\s+)", boost::regex_constants::perl);
      boost::regex done(R"(^\[x\])");
      boost::regex undone(R"(^\[\s?\])");
      boost::regex h1(R"(^#{1}\s([^\n#]+))", boost::regex_constants::perl);
      boost::regex h2(R"(^#{2}\s([^\n#]+))", boost::regex_constants::perl);
      boost::regex h3(R"(^#{3}\s([^\n#]+))", boost::regex_constants::perl);
      boost::regex h4(R"(^#{4}\s([^\n#]+))", boost::regex_constants::perl);
      boost::regex h5(R"(^#{5}\s([^\n#]+))", boost::regex_constants::perl);
      boost::regex h6(R"(^#{6}\s([^\n#]+))", boost::regex_constants::perl);

      boost::regex tables(R"(^ *\|.*\| *\n^ *\| *(:?-+:? *\|)+ *\n(^ *\|.*\| *\n)*)");

      // === Colors / formatting ===
      const std::string reset = "\x1b[0m";
      const std::string bold = "\x1b[1m";
      const std::string underline = "\x1b[4m";
      const std::string green = "\x1b[32m";
      const std::string blue = "\x1b[34m";
      const std::string cyan = "\x1b[36m";
      const std::string gray = "\x1b[90m";
      const std::string bg_gray = "\x1b[100m";
      const std::string bg_yellow = "\x1b[43m";
      const std::string strikethr = "\x1b[9m";

      const std::string header_colors[6] = {
        "\x1b[38;5;141m",
        "\x1b[38;5;135m",
        "\x1b[38;5;99m",
        "\x1b[38;5;74m",
        "\x1b[38;5;37m",
        "\x1b[38;5;31m"
      };

      // === Prepare separator line ===
      std::string separator;
      for (int i = 0; i < get_terminal_width(); i++) {
        separator += "─";
      }

      // === Start highlighting pipeline ===
      std::string highlighted = ttu;

      // Order matters — images before links
      highlighted = boost::regex_replace(highlighted, imgs, "[IMG] " + green + "$1 " + reset + blue + "($2)" + reset);
      highlighted = boost::regex_replace(
          highlighted,
          links,
          osc8_start + "$2" + "\x1b\\" + green + "$1" + reset + osc8_end + " " + blue + " ($2)" + reset
      );

      highlighted = boost::regex_replace(highlighted, hyperlinks, blue + underline + "$1" + reset);

      highlighted = boost::regex_replace(highlighted, separators, separator);
      highlighted = boost::regex_replace(highlighted, bld, cyan + bold + "$1" + reset);
      highlighted = boost::regex_replace(highlighted, italic, yellow + "\x1b[3m$1$2" + reset);
      highlighted = boost::regex_replace(highlighted, inline_code, bg_gray + green + "$1" + reset);
      highlighted = boost::regex_replace(highlighted, underl, red + underline + "$1" + reset);
      highlighted = boost::regex_replace(highlighted, blockquotes, gray + "┃" + reset + "$1");
      highlighted = boost::regex_replace(highlighted, striketh, strikethr + "$1" + reset);
      highlighted = boost::regex_replace(highlighted, highl, bg_yellow + "$1" + reset);

      highlighted = boost::regex_replace(highlighted, done, green + "\U000f0133" + reset);
      highlighted = boost::regex_replace(highlighted, undone, "\U000f0130");

      highlighted = boost::regex_replace(highlighted, nums_list, "  $0");
      highlighted = boost::regex_replace(highlighted, unordered, "  • ");

      // Headers
      highlighted = boost::regex_replace(highlighted, h1, header_colors[0] + "$1" + reset);
      highlighted = boost::regex_replace(highlighted, h2, header_colors[1] + "$&" + reset);
      highlighted = boost::regex_replace(highlighted, h3, header_colors[2] + "$&" + reset);
      highlighted = boost::regex_replace(highlighted, h4, header_colors[3] + "$&" + reset);
      highlighted = boost::regex_replace(highlighted, h5, header_colors[4] + "$&" + reset);
      highlighted = boost::regex_replace(highlighted, h6, header_colors[5] + "$&" + reset);

      // === Rendering ===
      int scroll_offset = 0;
      int height = get_terminal_height();
      std::vector<std::string> lines = io::split(highlighted, "\n");

      auto redraw = [&](int sig = 0) {
        clear();
        for (int i = scroll_offset; i < std::min(scroll_offset + height, (int)lines.size()); i++) {
          io::print(lines[i] + "\n");
        }
        draw_statusbar(scroll_offset + get_terminal_height(), lines);
      };

      redraw();

      while (true) {
        char key;
        if (read(STDIN_FILENO, &key, 1) != 1) continue;

        if (key == 27) {
          char seq[2];
          if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
          if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

          switch (seq[1]) {
            case 'A': { // Up
              if (scroll_offset > 0) {
                scroll_offset--;
                redraw();
              }
              break;
            }
            case 'B': { // Down
              if (scroll_offset + get_terminal_height() < (int)lines.size()) {
                scroll_offset++;
                redraw();
              }
              break;
            }
            default: break;
          }
        }

        if (key == 'q') {
          clear();
          switch_to_normal();
          disable_raw_mode();
          _exit(0);
        }
      }
      return 0;
    }

  public:
    Md() {}

    int exec(std::vector<std::string> args) {
      if (args.empty()) {
        io::print(R"(md: renders markdown in a pretty format
usage: markdown <file>
       markdown -t "text"
       cmd | markdown -t
       
flags:
  -t | --text: incoming input is text, useful for highlighting
  
note: some terminals might not support some formatting options, like bold or underline,
      so some text might appear unhighlighted.
)");
        return 0;
      }

      std::vector<std::string> valid_args = {
        "-t", "--text"
      };

      std::string a;
      bool is_filepath = true;

      for (auto &arg : args) {
        if (arg.starts_with("-") && !io::vecContains(valid_args, arg)) {
          info::error("Invalid argument \"" + arg + "\"");
          return -1;
        }

        if (arg == "-t" || arg == "--text") is_filepath = false;
        if (!arg.starts_with("-")) a = arg;
      }

      if (!is_filepath && a.empty()) {
        std::stringstream ss; ss << std::cin.rdbuf();
        a = ss.str();
      }

      return render(a, is_filepath);
    }
};

int main(int argc, char* argv[]) {
  Md md;
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return md.exec(args);
}
