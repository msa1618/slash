#include <unistd.h>
#include <sstream>
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../help_helper.h"
#include "../cmd_highlighter.h"

#include <fcntl.h>
#include <iostream>

class Encode {
  public:
    Encode() {}

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

    std::string to_base32(const std::string& input) {
        const std::string base32_index = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
        std::string result;

        int buffer = 0;       // stores bits
        int bits_left = 0;    // number of bits in buffer

        for (unsigned char c : input) {
            buffer = (buffer << 8) | c;
            bits_left += 8;

            while (bits_left >= 5) {
                result.push_back(base32_index[(buffer >> (bits_left - 5)) & 0x1F]);
                bits_left -= 5;
            }
        }

        if (bits_left > 0) { // handle remaining bits
            result.push_back(base32_index[(buffer << (5 - bits_left)) & 0x1F]);
        }

        // Add padding '=' to make length a multiple of 8
        while (result.size() % 8 != 0) {
            result.push_back('=');
        }

        return result;
    }


    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        io::print(get_helpmsg({
          "Prints text in Base64 and Base32",
          {
            "encode [option] <file-or-text>"
          },
          {
            {"-6", "--base-64", "Use base64"},
            {"-3", "--base-32", "Use base32"},
            {"-t", "--text", "The incoming argument is text (useful for piping)"}
          },
          {
            {"encode -6 file.txt", "Encodes file.txt in base64"},
            {"encode -3 -t \"slash is great!\"", "Encodes the argument into base32"}
          },
          "",
          "You can use redirection to overwrite or append the result to a file\n  e.g. " + highl("encode -6 data.txt > data.txt")
        }));
        return 0;
      }

      std::vector<std::string> validArgs = {
        "-6", "-3", "-t"
      };

      bool use_base64 = false;
      bool use_base32 = false;
      bool is_text = false;

      std::string text_to_encode;
      std::string filepath;

      for(auto& arg : args) {
        if(arg.starts_with('-') && !io::vecContains(validArgs, arg)) {
          info::error("Invalid argument \"" + arg + "\"");
          return -1;
        }

        if(arg == "-t" || arg == "--text") is_text = true;
        if(arg == "-6" || arg == "--base-64") use_base64 = true;
        if(arg == "-3" || arg == "--base-32") use_base32 = true;
      }

      for(auto& arg : args) {
        if(arg.starts_with("-")) continue;

        if(is_text) {
          text_to_encode += arg + ' ';
        } else filepath = arg;
      }

      if(is_text) {
        if(text_to_encode.empty()) {
          std::stringstream ss;
          ss << std::cin.rdbuf();
        }

        if(use_base64) {
          io::print(to_base64(text_to_encode));
        } else if(use_base32) {
          io::print(to_base32(text_to_encode));
        }
      } else {
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
        if(use_base64) {
          io::print(to_base64(content));
        } else if(use_base32) {
          io::print(to_base32(content));
        }
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
