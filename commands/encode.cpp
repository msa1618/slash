#include <unistd.h>
#include "../abstractions/iofuncs.h"
#include "../command.h"
#include <fcntl.h>

// Coming in version 1.5.0: the decode command

class Encode : public Command {
  public:
    Encode() : Command("encode", "converts plaintext into various radix encodings, like base64", "") {}

    std::string to_base64(std::string input) {
      std::string base64_index = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

      std::string result;
      int val = 0, valb = -6;
      for(auto& c : input) {
        val = (val << 8) + c;
        valb += 8;
        while(valb >= 0) {
          result.push_back(base64_index[(val >> valb) & 0x3F]);
          valb -= 6;
        }
      }

       if (valb > -6) result.push_back(base64_index[((val << 8) >> (valb + 8)) & 0x3F]);
        while(result.size() % 4 != 0) result.push_back('=');
        return result;
    }


    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        io::print("encode: converts text into various radix encodings\n"
                  "usage: encode [-flag] <text>\n"
                  "flags:\n"
                  "-b64: converts to base64\n"
                  "-f: input is a file\n");
        return 0;
      }

      std::vector<std::string> validArgs = {
        "-b64", "-f"
      };

      bool use_base64 = false;
      bool is_file = false;

      std::string text_to_encode;
      std::string filepath;

      for(auto& arg : args) {
        if(arg.starts_with('-') && !io::vecContains(validArgs, arg)) {
          io::print_err("\x1b[1m\x1b[31merror:\x1b[0m bad arguments\n");
          return -1;
        }

        if(arg == "-f") is_file = true;
        if(arg == "-b64") use_base64 = true;
      }

      for(auto& arg : args) {
        if(arg.starts_with("-")) continue;

        if(!is_file) {
          text_to_encode += arg + ' ';
        } else filepath = arg;
      }

      if(use_base64 && !is_file) {
        io::print(to_base64(text_to_encode));
      } else if(use_base64 && is_file) {
        int fd = open(filepath.c_str(), O_RDONLY);
        if(fd == -1) { perror("Failed to open file"); return -1; }

        char buffer[1024];
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if(bytesRead < 0) {
          perror("Failed to read file");
          return -1;
        }

        buffer[bytesRead] = '\0';

        std::string content = std::string(buffer);
        io::print(to_base64(content));
      }

      io::print("\n");
      return 0;

    }
  };

int main(int argc, char* argv[]) {
	Encode encode;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	encode.exec(args);
	return 0;
}
