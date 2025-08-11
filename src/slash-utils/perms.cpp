#include <sstream>
#include "../command.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <unistd.h>
#include <sys/stat.h>

class Perms : public Command {
  private:
    std::string process_code(int code_in_octal) {
      std::stringstream ss;
      ss << 0 << std::oct << code_in_octal;
      std::string code_str = ss.str();
      if(code_str.length() != 4) {
        info::error("Invalid octal code. It should be a 3-digit octal number.");
        return "";
      }
      std::stringstream output;
      output << blue << "Octal code: " << reset << code_str << "\n\n";
      output << blue << "Human-readable format: " << reset;

      std::string human_readable;

      for(int i = 1; i < code_str.length(); i++) {
        char c = code_str[i];
        if(c == '0') {
          human_readable += "---";
        } else if(c == '1') {
          human_readable += "--x";
        } else if(c == '2') {
          human_readable += "-w-";
        } else if(c == '3') {
          human_readable += "-wx";
        } else if(c == '4') {
          human_readable += "r--";
        } else if(c == '5') {
          human_readable += "r-x";
        } else if(c == '6') {
          human_readable += "rw-";
        } else if(c == '7') {
          human_readable += "rwx";
        }
      }

      output << human_readable << "\n";
      output << blue << "Format:" << reset << " [owner][group][other]\n\n";

      output << blue << "Truly human-readable format: \n" << reset;
      output << blue << "  - Owner: " << reset;
      std::string owner;
      for(auto& c : human_readable.substr(0, 3)) {
        if(c == 'r') owner += "Read ";
        else if(c == 'w') owner += "Write ";
        else if(c == 'x') owner += "Execute ";
      }
      output << owner << "\n";

      output << blue << "  - Group: " << reset;
      std::string group;
      for(auto& c : human_readable.substr(3, 3)) {
        if(c == 'r') group += "Read ";
        else if(c == 'w') group += "Write ";
        else if(c == 'x') group += "Execute ";
      }
      output << group << "\n";

      output << blue << "  - Other: " << reset;
      std::string other;
      for(auto& c : human_readable.substr(6, 3)) {
        if(c == 'r') other += "Read ";
        else if(c == 'w') other += "Write ";
        else if(c == 'x') other += "Execute ";
      }
      output << other << "\n";

      return output.str();
    }

    int change_permissions(std::string path, std::string mode) {
      if(std::all_of(mode.begin(), mode.end(), ::isdigit)) {
        int num = 0;
        try {
          num = std::stoi(mode, nullptr, 8);
        } catch(...) {
          info::error("Invalid number! The number should be a 3 digit octal number starting with 0");
          return -1;
        }

        if(chmod(path.c_str(), num) != 0) {
          info::error(std::string("Failed to change permissions: ") + strerror(errno), errno);
          return errno;
        }
        return 0;
      }

      if(mode.length() != 9) {
        info::error("Invalid mode! Mode should be 9 characters long");
        return 0;
      }

      auto to_digit = [](const std::string& triplet) {
          int val = 0;
          if (triplet[0] == 'r') val += 4;
          if (triplet[1] == 'w') val += 2;
          if (triplet[2] == 'x') val += 1;
          return val;
      };

      int user = to_digit(mode.substr(0, 3));
      int group = to_digit(mode.substr(3, 3));
      int other = to_digit(mode.substr(6, 3));

      mode_t octal_mode = (user << 6) | (group << 3) | other;

      if (chmod(path.c_str(), octal_mode) != 0) {
          info::error(std::string("Failed to change permissions: ") + strerror(errno), errno);
          return errno;
      }

      return 0;
    }

  public:
    Perms() : Command("perms", "", "") {}

    int exec(std::vector<std::string> args) override {
     if (args.empty()) {
        io::print("perms: set, and view file permissions\n"
                  "usage: perms <file> [mode]\n"
                  "flags:\n"
                  "-g | --get:     get the current permissions of a file\n"
                  "-s | --set:     set the permissions of a file\n"
                  "-p | --process: process an octal code to a human-readable format\n");
        return 0;
     }

      std::vector<std::string> valid_args = {
        "-g", "-s", "-p",
        "--get", "--set", "--process"
      };

      std::string path;
      std::string mode;

      for(auto& a : args) {
        if(!io::vecContains(valid_args, a) && a.starts_with('-')) {
          info::error("Invalid argument: " + a);
          return EINVAL;
        }

        if(a == "-g" || a == "--get") {
          if(args.size() < 2) {
            info::error("No file specified for getting permissions");
            return EINVAL;
          }
          path = args[1];
          struct stat st;
          if(stat(path.c_str(), &st) != 0) {
            std::string error = std::string("Failed to get permissions: ") + strerror(errno);
            info::error(error, errno, path);
            return errno;
          }
          io::print(process_code(st.st_mode & 0777));
          return 0;
        }

      if(a == "-s" || a == "--set") {
        if(args.size() < 3) {
          info::error("No file or mode specified for setting permissions");
          return EINVAL;
        }
        path = args[1];
        mode = args[2];
        return change_permissions(path, mode);
      }

      if(a == "-p" || a == "--process") {
        if(args.size() < 2) {
          info::error("No octal code specified for processing");
          return EINVAL;
        }
        if(!std::all_of(args[1].begin(), args[1].end(), ::isdigit)) {
          info::error("Code should be a number");
          return EINVAL;
        }
        int code_in_octal = std::stoi(args[1], nullptr, 8);
        io::print(process_code(code_in_octal));
        return 0;
      }
    }
  }
  };

int main(int argc, char* argv[]) {
  Perms perms;
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  return perms.exec(args);
}