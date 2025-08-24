#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../help_helper.h"


class Eol {
  private:
    int change_eol(std::string filepath, std::string eol) {
      auto content = io::read_file(filepath);
      if(!std::holds_alternative<std::string>(content)) {
        std::string error = std::string("Failed to read file: ") + strerror(errno);
        info::error(error, errno);
        return errno;
      }

      std::string new_content = std::get<std::string>(content);
      // Normalize all EOLs to LF first
      io::replace_all(new_content, "\r\n", "\n");
      io::replace_all(new_content, "\r", "\n");

      io::replace_all(new_content, "\n", eol);
      if(io::overwrite_file(filepath, new_content) != 0) {
        std::string error = std::string("Failed to overwrite file: ") + strerror(errno);
        info::error(error, errno);
        return errno;
      }
    }

    int count_occurences(std::string content, std::string str) {
      size_t pos = 0;
      int count = 0;

      while((pos = content.find(str, pos)) != std::string::npos) {
        count++;
        pos += str.length();
      }

      return count;
    }

    int print_occurences(std::string filepath, bool seq) {
      auto content = io::read_file(filepath);
      if (!std::holds_alternative<std::string>(content)) {
        info::error("Failed to read file: " + std::string(strerror(errno)), errno);
        return errno;
      }

      std::string cnt = std::get<std::string>(content);

      std::string crlf = std::to_string(count_occurences(cnt, "\r\n"));
      io::replace_all(cnt, "\r\n", ""); // remove CRLFs
      std::string lf = std::to_string(count_occurences(cnt, "\n"));
      std::string cr = std::to_string(count_occurences(cnt, "\r"));

      std::stringstream ss;

      std::string color;

      if(seq) {
        io::print(lf + ", " + crlf + ", " + cr);
      } else {
        io::print(orange + "LF: " + reset + lf + "\n");
        io::print(blue   + "CRLF: " + reset + crlf + "\n");
        io::print(cyan   + "CR: " + reset + cr + "\n");

      }

      return 0;
    }

    public:
      Eol() {}

      int exec(std::vector<std::string> args) {
        if(args.empty()) {
          io::print(get_helpmsg({
            "Print and manipulate end-of-lines of a file",
            {
              "eol <file>",
              "eol [option] <file>"
            },
            {
              {"-u", "--unix", "Converts EOL to LF"},
              {"-w", "--windows", "Converts EOL to CRLF"},
              {"-m", "--macintosh", "Converts EOL to CR, used in old Macs"},
              {"-s", "--seq", "Print occurences in the format {lf, crlf, cr}"}
            },
            {
              {"eol main.cpp", "Prints the occurences of the 3 main EOLs of main.cpp"},
              {"eol -w main.cpp", "Change the EOL of main.cpp to Windows (CRLF)"},
            }
          }));
            return 0;
        }

        std::vector<std::string> valid_args = {
          "-u", "-w", "-m", "-s",
          "--unix", "--windows", "--mac", "--seq"
        };

        std::string filepath;

        bool unix_eol = false;
        bool windows  = false;
        bool mac      = false;
        bool seq      = false;

        for(auto& arg : args) {
          if(arg.starts_with("-") && !io::vecContains(valid_args, arg)) {
            info::error("Invalid argument \"" + arg + "\"");
            return -1;
          }

          if(arg == "-u" || arg == "--unix") unix_eol = true;
          if(arg == "-w" || arg == "--windows") windows = true;
          if(arg == "-m" || arg == "--mac") mac = true;
          if(arg == "-s" || arg == "--seq") seq = true;

          if(!arg.starts_with("-")) filepath = arg;
        }

        bool print_occurs = !(unix_eol || windows || mac);

        if(print_occurs) return print_occurences(filepath, seq);
        if(unix_eol) return change_eol(filepath, "\n");
        if(windows) return change_eol(filepath, "\r\n");
        if(mac) return change_eol(filepath, "\r");
      }
};

int main(int argc, char* argv[]) {
  Eol eol;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  return eol.exec(args);
}