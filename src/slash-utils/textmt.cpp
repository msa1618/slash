
#include <unordered_map>
#include <string>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../help_helper.h"

class Textmt {
  private:
  std::string get_chars_num(std::string str) {
    int num = 0;
    for (auto &c : str) num++;
    return std::to_string(num);
  }

  std::string get_words_num(std::string str) {
    std::vector<std::string> words = io::split(str, " ");
    return std::to_string(words.size());
  }

  std::string get_lines_num(std::string str) {
    std::vector<std::string> lines = io::split(str, "\n");
    return std::to_string(lines.size());
  }

  std::string get_alphachars_num(std::string str) {
    int amount = 0;
    for (auto &c : str) {
      if (isalpha(c)) amount++;
    }
    return std::to_string(amount);
  }

  std::string get_digits_num(std::string str) {
    int amount = 0;
    for (auto &c : str) {
      if (isdigit(c)) amount++;
    }
    return std::to_string(amount);
  }

  std::string get_symbols_num(std::string str) {
    int amount = 0;
    for (auto &c : str) {
      if (!isalnum(c) && !isspace(c) && !iscntrl(c)) amount++;
    }
    return std::to_string(amount);
  }

  std::string get_spaces_num(std::string str) {
    int amount = 0;
    for (auto &c : str) {
      if (isspace(c)) amount++;
    }
    return std::to_string(amount);
  }


  std::string most_used_char(std::string str) {
    std::unordered_map<char, int> freq;
    for (auto &c: str) freq[c]++;

    std::string most_common = "\0";
    int mc_num = 0;

    for (auto &[ch, num]: freq) {
      if (num > mc_num) {
        most_common = std::string(1, ch);
        mc_num = num;
      }
    }

    if(most_common == " ") most_common = "<space>";
    if(most_common == "\n") most_common = "<newline>";
    if(most_common == "\t") most_common = "<tab>";

    return most_common;
  }

  void print_metadata(std::string str, std::string filename = "") {
    if (!filename.empty()) {
      io::print(blue + "Filename: " + reset + filename + "\n");
    }
    io::print(blue + "Letters: " + reset + get_chars_num(str) + "\n");
    io::print(blue + "Words:   " + reset + get_words_num(str) + "\n");
    io::print(blue + "Lines:   " + reset + get_lines_num(str) + "\n");
    io::print(blue + "Number of: \n" + reset);
    io::print(blue + "  Alphabet letters: " + reset + get_alphachars_num(str) + "\n");
    io::print(blue + "  Numbers:    " + reset + get_digits_num(str) + "\n");
    io::print(blue + "  Symbols:    " + reset + get_symbols_num(str) + "\n");
    io::print(blue + "  Whitespace: " + reset + get_spaces_num(str) + "\n");

    std::string mu_char = most_used_char(str);
    io::print(blue + "Most used char: " + reset + most_used_char(str) + "\n");
  }

  public:
  Textmt() {}

  int exec(std::vector<std::string> args) {
    if (args.empty()) {
      io::print(get_helpmsg({
        "Prints various metadata of text/text in a file",
        {
          "textmt <filepath>",
          "textmt [option] <text>"
        },
        {
          {"-t", "--text", "The next argument is text, not a file. Used for piping"}
        },
        {
          {"textmt main.cpp", "Prints text metadata about main.cpp"},
          {"textmt -t \"Hi Mom!\"", "Prints text metadata about the argument"}
        },
        "",
        ""
      }));
      return 0;
    }

    std::vector<std::string> valid_args = {
      "-t",
      "--text"
    };

    bool is_text = false;
    std::string arg;

    for (auto& a : args) {
      if (!io::vecContains(valid_args, a) && a.starts_with("-")) {
        info::error("Invalid argument: \"" + a + "\"!\n");
        return EINVAL;
      }

      if (a == "-t" || a == "--text") {
        is_text = true;
        continue;
      }

      if (!a.starts_with("-")) arg = a;
    }

    if (is_text) {
      if(arg.empty()) {
        std::stringstream ss;
        ss << std::cin.rdbuf();
        arg = ss.str();
      }
      print_metadata(arg);
    } else {
      auto content = io::read_file(arg);
      if(!std::holds_alternative<std::string>(content)) {
        std::string error = std::string("Failed to read file: ") + strerror(errno);
        info::error(error, errno);
        return errno;
      }
      std::string str = std::get<std::string>(content);
      print_metadata(str, arg);
    }
    return 0;
  }
};

int main(int argc, char* argv[]) {
  Textmt textmt;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  textmt.exec(args);
  return 0;
}