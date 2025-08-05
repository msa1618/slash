#include <iostream>
#include <sstream>
#include <iomanip>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../command.h"

class Dump : public Command {
  private:
    int dump(bool is_file, bool oct, std::string input) {
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
        if(!oct) ss << std::hex;
        else ss << std::oct;
        ss << bytesRead << ": " << reset;
        
        for(size_t j = 0; j < 16; ++j) {
          if(i + j < text.size()) {
            unsigned char c = text[i + j];
            int width;
            if(!oct) width = 2;
            else width = 3;

            ss << std::setw(width) << std::setfill('0') << std::uppercase;
            if(!oct) ss << std::hex; //std::hex << (int)c << " ";
            else ss << std::oct;
            ss << (int)c << " ";
          } else {
            ss << "   "; // pad for missing bytes
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
      Dump() : Command("dump", "", "") {}

      int exec(std::vector<std::string> args) {
        if(args.empty()) {
          io::print(R"(dump: dump a file or text in either hex or oct
usage: hex <filename> 

flags:
  -o | --octal: dump in octal
  -t | --text:  input is text
)");
            return 0;
        }

        std::vector<std::string> valid_args = {
          "-t", "-o",
          "--text", "--octal"
        };

        bool is_file = true;
        bool use_octal = false;
        std::string a;

        for(auto& arg : args) {
          if(!io::vecContains(valid_args, arg) && arg.starts_with("-")) {
            info::error("Invalid argument: \"" + arg + "\"");
            return -1;
          }

          if(arg == "-t" || arg == "--text") is_file = false;
          if(arg == "-o" || arg == "--octal") use_octal = true;
          if(!arg.starts_with("-")) a = arg;
        }

        if(!is_file) {
          if(!a.empty()) return dump(false, use_octal, a);
          else { // The argument is empty, so the user could be piping
            std::stringstream piped;
            piped << std::cin.rdbuf();
            return dump(false, use_octal, piped.str());
          }
        } else return dump(true, use_octal, a);
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