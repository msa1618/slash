#include <iostream>
#include <sstream>
#include <iomanip>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../help_helper.h"
#include <bitset>

class Dump {
  private:
    enum class Type {
      Hex,
      Octal,
      Decimal,
      Binary
    };

    int dump(bool is_file, Type mode, std::string input) {
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
      int no_of_bytes = mode == Type::Binary ? 6 : 16;

      for(int i = 0; i < text.size(); i += no_of_bytes) {
        std::stringstream ss;
        ss << magenta << std::setw(7) << std::setfill('0');
        if(mode == Type::Hex || mode == Type::Binary) ss << std::hex;
        else if(mode == Type::Octal) ss << std::oct;
        else if(mode == Type::Decimal) ss << std::dec;
        ss << bytesRead << ": " << reset;
        
        for(size_t j = 0; j < no_of_bytes; ++j) {
          if(i + j < text.size()) {
            unsigned char c = text[i + j];
            int width;
            if(mode == Type::Hex) width = 2;
            else if(mode == Type::Binary) width = 8;
            else width = 3;

            ss << std::setw(width) << std::setfill('0') << std::uppercase;
            if(mode == Type::Binary) {
              std::bitset<8> ch(c);
              ss << ch.to_string() << " ";
            } else {
              if(mode == Type::Hex) ss << std::hex;
              else if(mode == Type::Octal) ss << std::oct;
              else if(mode == Type::Decimal) ss << std::dec;
              ss << (int)c << " ";
            }
          } else {
            // pad for missing bytes
            if(mode == Type::Hex) ss << "   ";
            else if(mode == Type::Binary) ss << "         ";
            else ss << "    ";
          }
        }

        for(size_t j = 0; j < no_of_bytes && (i + j) < text.size(); ++j) {
            unsigned char c = text[i + j];
            ss << green << (std::isprint(c) ? static_cast<char>(c) : '.') << reset;
        }

        io::print(ss.str() + "\n");
        bytesRead += no_of_bytes;
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
              {"-b", "--bin", "Dump in binary"},
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
          "--text", "--octal", "--decimal", "--bin"
        };

        bool is_file = true;
        Type mode = Type::Hex;
        std::string a;

        for(auto& arg : args) {
          if(!io::vecContains(valid_args, arg) && arg.starts_with("-")) {
            info::error("Invalid argument: \"" + arg + "\"");
            return -1;
          }

          if(arg == "-t" || arg == "--text") is_file = false;
          if(arg == "-b" || arg == "--bin") mode = Type::Binary;
          if(arg == "-o" || arg == "--octal") mode = Type::Octal;
          if(arg == "-d" || arg == "--decimal") mode = Type::Decimal;
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