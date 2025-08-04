#include <iostream>
#include <sstream>
#include <iomanip>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../command.h"

class Hex : public Command {
  private:
    int dump(bool is_file, std::string input) {
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
        ss << magenta << std::setw(7) << std::setfill('0') << std::hex << bytesRead << ": " << reset;
        
        for(size_t j = 0; j < 16; ++j) {
          if(i + j < text.size()) {
            unsigned char c = text[i + j];
            ss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << (int)c << " ";
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
      Hex() : Command("hex", "", "") {}

      int exec(std::vector<std::string> args) {
        if(args.empty()) {
          io::print(R"(hex: hexdump a file or text
usage: hex <filename> 

flags:
  -t | --text: input is text
)");
            return 0;
        }

        std::vector<std::string> valid_args = {
          "-t",
          "--text"
        };

        bool is_file = true;
        std::string a;

        for(auto& arg : args) {
          if(!io::vecContains(valid_args, arg) && arg.starts_with("-")) {
            info::error("Invalid argument: \"" + arg + "\"");
            return -1;
          }

          if(arg == "-t" || arg == "--text") is_file = false;
          if(!arg.starts_with("-")) a = arg;
        }

        if(!is_file) {
          if(!a.empty()) return dump(false, a);
          else { // The argument is empty, so the user could be piping
            std::stringstream piped;
            piped << std::cin.rdbuf();
            return dump(false, piped.str());
          }
        } else return dump(true, a);
      }
};

int main(int argc, char* argv[]) {
	Hex hex;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	hex.exec(args);
	return 0;
}