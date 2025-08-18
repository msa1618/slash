#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <cmath>


#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../help_helper.h"


class Echo {
  private:
    std::string to_uppercase(std::string& text) {
      for(auto& c : text) {
        c = toupper(c);
      }
      return text;
    }

  std::string to_lowercase(std::string& text) {
    for(auto& c : text) {
      c = tolower(c);
    }
    return text;
  }

  std::string to_weirdcase(std::string& text) {
    std::string output;
    bool uppercase = false;

    for(char c : text) {
      if(uppercase) output += toupper(c);
      else output += tolower(c);
      uppercase = !uppercase;
    }

    return output;
  }

  std::string get_ansi_rgb(int r, int g, int b) {
    std::stringstream result;
    result << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
    return result.str();
  }

  std::string to_rainbow(std::string text) {
    int length = text.size();
    std::stringstream ss;

    for (int i = 0; i < length; ++i) {
      double ratio = (double)i / length;
      // Use sine waves to get smooth rainbow colors
      int r = (int)(std::sin(ratio * 2 * M_PI) * 127 + 128);
      int g = (int)(std::sin(ratio * 2 * M_PI + 2 * M_PI / 3) * 127 + 128);
      int b = (int)(std::sin(ratio * 2 * M_PI + 4 * M_PI / 3) * 127 + 128);

      ss <<  get_ansi_rgb(r, g, b);
      ss << std::string(1, text[i]);
    }
    ss << reset;
    return ss.str();
  }

  int echo(std::string text, bool uppercase, bool lowercase, bool weirdcase, bool infinite, bool stderr_, bool rainbow, bool typewriter, bool reverse, int loopnum) {
      if(uppercase) text = to_uppercase(text);
      if(lowercase) text = to_lowercase(text);
      if(weirdcase) text = to_weirdcase(text);

      if(rainbow) text = to_rainbow(text);
      if(reverse) std::reverse(text.begin(), text.end());

      for(int i = 0; i < loopnum; i++) {
          if(typewriter) {
            for(auto& c : text) {
              if(stderr_) io::print_err(std::string(1, c));
              else io::print(std::string(1, c));
              std::this_thread::sleep_for(std::chrono::milliseconds(75));
            }
          } else {
            if(stderr_) io::print_err(text);
            else io::print(text);
          }
      }

      if(infinite) {
        while(true) {
          if(typewriter) {
            for(auto& c : text) {
              if(stderr_) io::print_err(std::string(1, c));
              else io::print(std::string(1, c));
              std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
          } else {
            if(stderr_) io::print_err(text);
            else io::print(text);
          }
        }
      }

      io::print("\n");
      return 0;
     }

  public:
    Echo() {}

  int exec(std::vector<std::string> args) {
    if(args.empty()) {
      io::print(get_helpmsg({
        "Prints back whatever arguments you type in",
        {
          "echo <args>",
          "echo [option] <args>"
        },
        {
          {"-l", "--lowercase", "prints in lowercase"},
          {"-u", "--uppercase", "PRINTS IN UPPERCASE"},
          {"-w", "--weirdcase", "pRiNtS iN wEiRdCaSe"},
          {"-r", "--rainbow", to_rainbow("Prints in rainbow")},
          {"-e", "--error", "Prints to stderr"},
          {"-R", "--reverse", "Reverse text"},
          {"-L", "--loop [n]: Prints arguments (n) number of times"},
          {"", "--typewriter", "Prints arguments kinda slowly, like a typewriter"}
        },
        {
          {"echo \"Hello World!\"", "Simply prints Hello World"},
          {"echo -L 3 \"This text is printed 3 times\"", "Prints the arguments three times"},
          {"echo -r \"nyan cat\"", "Prints the arguments in rainbow"}
        },
        "",
        ""
      }));
        return 0;
    }

    std::vector<std::string> valid_args = {
        "-l", "--lowercase",
        "-u", "--uppercase",
        "-w", "--weirdcase",
        "-r", "--rainbow",
        "-e", "--error",
        "-R", "--reverse",
        "-L", "--loop",
        "--typewriter"
    };

    std::vector<std::string> text;
    bool infinite = false;

    int loopCount = 1;

    bool uppercase   = false;
    bool lowercase   = false;
    bool weirdcase   = false;
    bool reverse     = false;
    bool use_rainbow = false;
    bool stderr_     = false;
    bool typewriter  = false;

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "-u" || arg == "--uppercase") uppercase = true;
        else if (arg == "-l" || arg == "--lowercase") lowercase = true;
        else if (arg == "-w" || arg == "--weirdcase") weirdcase = true;
        else if (arg == "-r" || arg == "--rainbow") use_rainbow = true;
        else if (arg == "-e" || arg == "--error") stderr_ = true;
        else if (arg == "-R" || arg == "--reverse") reverse = true;
        else if (arg == "--typewriter") typewriter = true;
        else if (arg == "-i" || arg == "--infinite") infinite = true;
        else if (arg == "-L" || arg == "--loop") {
            // handle loop count
            if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                try {
                  loopCount = std::stoi(args[++i]);
                } catch(...) {
                  info::error("Loop argument must be a number.");
                  return -1;
                }
            } else {
                info::error("Missing loop number!");
                return -1;
            }
        } else {
            text.push_back(arg);
        }
    }

    if((uppercase || lowercase || weirdcase || infinite || stderr_ || use_rainbow || typewriter || reverse || loopCount > 1) && text.empty()) {
      // Could be piped to
      std::stringstream ss;
      ss << std::cin.rdbuf();
      text.push_back(ss.str());
    }

    return echo(io::join(text, " "), uppercase, lowercase, weirdcase, infinite, stderr_, use_rainbow, typewriter, reverse, loopCount);
  }
};

int main(int argc, char* argv[]) {
  Echo echoObj;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  int ret = echoObj.exec(args);
  return ret;
}