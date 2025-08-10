#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <vector>
#include <optional>
#include <sstream>

struct FigletFont {
  std::vector<std::string> lines;
  int height;
  int start;
  char hardblank;
};

class Acart {
  private:
    std::optional<FigletFont> load_font(const std::string& figlet_path) {
      auto content = io::read_file(figlet_path);
      if (!std::holds_alternative<std::string>(content)) return std::nullopt;

      std::vector<std::string> lines = io::split(std::get<std::string>(content), "\n");
      if (lines.empty()) return std::nullopt;

      std::vector<std::string> metadata = io::split(lines[0], " ");
      char hardblank = metadata[0].back();
      int height = std::stoi(metadata[1]);
      int comment_lines = std::stoi(metadata[5]);
      int start = 1 + comment_lines;

      return FigletFont{lines, height, start, hardblank};
    }

    std::vector<std::string> get_ascii_char_lines(char c, const FigletFont& font) {
      int char_index = static_cast<int>(c) - 32;
      int start_line = font.start + char_index * font.height;
      std::vector<std::string> result;

      for (int i = 0; i < font.height; ++i) {
        if (start_line + i >= (int)font.lines.size()) break;
        std::string line = font.lines[start_line + i];
        if (!line.empty()) line.pop_back(); // remove endmark

        for (char& ch : line) {
          if (ch == font.hardblank) ch = ' ';
        }
        result.push_back(line);
      }
      
      return result;
    }

  public:
    Acart() {}

    int exec(std::vector<std::string> args) {
      if (args.empty()) {
        io::print(R"(acart: generate ascii art
usage: acart <text>                 use default font
       acart -f <filepath> <text>   get characters from a FIGlet font

flags:
  -f | --file: next argument is a file
)");
        return 0;
      }

      std::vector<std::string> valid_args = {"-f", "--file"};

      std::string file;
      std::string text;

      for (int i = 0; i < (int)args.size(); i++) {
        std::string arg = args[i];
        if (arg.starts_with("-") && !io::vecContains(valid_args, arg)) {
          info::error("Invalid argument \"" + arg + "\"");
          return EINVAL;
        }

        if (arg == "-f" || arg == "--file") {
          if (i + 1 >= (int)args.size()) {
            info::error("Missing filename.");
            return EINVAL;
          }
          file = args[++i];
        } else if (!arg.starts_with("-")) {
          text = arg;
        }
      }

      if (file.empty()) {
        info::error("No font file specified.");
        return EINVAL;
      }

      auto font_opt = load_font(file);
      if (!font_opt) {
        info::error("Failed to load font file.");
        return -1;
      }
      FigletFont font = *font_opt;

      std::vector<std::string> output(font.height, "");

      for (char c : text) {
        auto ch_lines = get_ascii_char_lines(c, font);
        if ((int)ch_lines.size() != font.height) {
          info::error("Invalid character or font data.");
          return -1;
        }
        for (int i = 0; i < font.height; ++i) {
          output[i] += ch_lines[i];
        }
      }

      std::stringstream ss;
      for (auto& line : output) {
        ss << line << "\n";
      }
      io::print(ss.str());

      return 0;
    }
};

int main(int argc, char* argv[]) {
  Acart acart;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  return acart.exec(args);
}
