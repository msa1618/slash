#include <iostream>
#include <sstream>
#include <iomanip>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../help_helper.h"

class Dump {
  private:
    enum Types {
      Hex,
      Octal,
      Decimal
    };

    int dump(bool is_file, int mode, std::string input) {
      std::string text;

      if(is_file) {
        auto content = io::read_file(input);
        if(!std::holds_alternative<std::string>(content)) {
          std::string error = std::string("Failed to read " + input + ": ") + strerror(errno);
          info::error(error, errno);
          return -1;
        }

        text = std::get<std::string>(content);
      } else text = input;

      int bytesRead = 0;
      for(int i = 0; i < text.size(); i += 16) {
        std::stringstream ss;
        ss << magenta << std::setw(7) << std::setfill('0');
        if(mode == Hex) ss << std::hex;
        else if(mode == Octal) ss << std::oct;
        else if(mode == Decimal) ss << std::dec;
        ss << bytesRead << ": " << reset;
        
        for(size_t j = 0; j < 16; ++j) {
          if(i + j < text.size()) {
            unsigned char c = text[i + j];
            int width;
            if(mode == Hex) width = 2;
            else width = 3;

            ss << std::setw(width) << std::setfill('0') << std::uppercase;
            if(mode == Hex) ss << std::hex;
            else if(mode == Octal) ss << std::oct;
            else if(mode == Decimal) ss << std::dec;
            ss << (int)c << " ";
          } else {
            if(mode == Hex) ss << "   "; // pad for missing bytes
            else ss << "    ";
          }
        }

        for(size_t j = 0; j < 16 && (i + j) < text.size(); ++j) {
            unsigned char c = text[i + j];
            ss << green << (std::isprint(c) ? static_cast<char>(c) : '.') << reset;
        }

        io::print(ss.str() + "\n");
        bytesRead += 16;
      }
      return 0;
    }

    public:
      Dump() {}

      int exec(std::vector<std::string> args) {
        if(args.empty()) {
          io::print(get_helpmsg({
            "Dump text or a file in either hexadecimal, octal, decimal\n  or binary",
            {
              "dump <file>"
              "dump [option] <file-or-text>"
            },
            {
              {"-o", "--octal", "Dump in octal"},
              {"-d", "--decimal", "Dump in decimal"},
              {"-b", "--binary", "Dump in binary"},
              {"-t", "--text", "The next argument is text (used for piping)"}
            },
            {
              {"dump data.bin", "Show a hexdump of data.bin"},
              {"dump -o main.cpp", "Show an octdump of main.cpp"}
            },
            "",
            ""
          }));
            return 0;
        }

        std::vector<std::string> valid_args = {
          "-t", "-o", "-d", "-b",
          "--text", "--octal", "--decimal", "--binary"
        };

        bool is_file = true;
        int mode = Hex;
        std::string a;

        for(auto& arg : args) {
          if(!io::vecContains(valid_args, arg) && arg.starts_with("-")) {
            info::error("Invalid argument: \"" + arg + "\"");
            return -1;
          }

          if(arg == "-t" || arg == "--text") is_file = false;
          if(arg == "-o" || arg == "--octal") mode = Octal;
          if(arg == "-d" || arg == "--decimal") mode = Decimal;
          if(!arg.starts_with("-")) a = arg;
        }

        if(!is_file) {
          if(!a.empty()) return dump(false, mode, a);
          else { // The argument is empty, so the user could be piping
            std::stringstream piped;
            piped << std::cin.rdbuf();
            return dump(false, mode, piped.str());
          }
        } else return dump(true, mode, a);
      }
};

int main(int argc, char* argv[]) {
  Dump dump;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  dump.exec(args);
  return 0;
}