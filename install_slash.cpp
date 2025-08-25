#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>
#include <nlohmann/json.hpp>

const std::string black   = "\033[30m";
const std::string red     = "\033[31m";
const std::string green   = "\033[32m";
const std::string yellow  = "\033[33m";
const std::string blue    = "\033[34m";
const std::string magenta = "\033[35m";
const std::string cyan    = "\033[36m";
const std::string white   = "\033[37m";

const std::string reset   = "\033[0m";

// If you're coming from the slash source code, and wondering why functions from iofuncs are not used, its because they're not compiled duh
void print(std::string str) {
  write(STDOUT_FILENO, str.c_str(), str.size());
}

void print_stderr(std::string str) {
  write(STDERR_FILENO, str.c_str(), str.size());
}

void error(std::string error, bool print_strerror = false) {
  std::string red = "\x1b[31m";
  print_stderr(red + "[Error] " + reset + error + "\n");
  if(print_strerror) print_stderr(strerror(errno));
}

int create_file(std::string name, bool dir) {
  if(!dir) {
    int fd = open(name.c_str(), O_WRONLY | O_CREAT, 0744);
    if(fd < 0) {
      error("Failed to create file: " + name + " (" + strerror(errno) + ")");
      return errno;
    }
    close(fd);
  } else {
    if(mkdir(name.c_str(), 0755) < 0) {
      if(errno != EEXIST) {
        error("Failed to create directory: " + name + " (" + strerror(errno) + ")");
        return errno;
      }
    }
  }
  return 0;
}

int overwrite_file(const std::string& path, const std::string& content) {
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
      error(strerror(errno));
      return errno;
    }

    ssize_t written = write(fd, content.c_str(), content.size());
    if (written < 0) { close(fd); return errno; }
    if ((size_t)written < content.size()) { close(fd); return EIO; }

    close(fd);
    return 0;
}

std::vector<std::string> split(const std::string& s, std::string delimiter) {
  std::vector<std::string> tokens;
  size_t start = 0;
  size_t end;

  while ((end = s.find(delimiter, start)) != std::string::npos) {
    tokens.push_back(s.substr(start, end - start));
    start = end + delimiter.length();
  }
  tokens.push_back(s.substr(start));

  return tokens;
}

//////////////

int execute(std::string command) {
  pid_t pid = fork();
  if(pid < 0) {
    return errno;
  }

  if(pid == 0) {
    int devnull = open("/dev/null", O_RDWR);

    dup2(STDOUT_FILENO, devnull);
    dup2(STDERR_FILENO, devnull);

    std::vector<char*> argv;
    for(auto& token : split(command, " ")) argv.push_back(const_cast<char*>(token.c_str()));
    argv.push_back(nullptr); // NULL-terminate

    execvp(argv[0], argv.data());
  } else {
    int status = 0;
    waitpid(pid, &status, 0);
    if(WIFEXITED(status)) return WEXITSTATUS(status);
  }
}

//////////////

std::string get_settings() {
  std::string HOME = getenv("HOME");
  nlohmann::json j;
  j["pathOfPromptTheme"] = HOME + "/.slash/config/prompts/default.json";
  j["printExitCodeWhenProgramExits"] = false;

  return j.dump(2);
}

std::string get_default_json_content() {
  nlohmann::json j = {
    {"segment-style", "arrow"},
    {"padding", {
        {"enabled", true},
        {"spaces", 1}
    }},
    {"before-all", {
        {"enabled", true},
        {"chars", "┌─"}
    }},
    {"time", {
        {"enabled", false},
        {"before", ""},
        {"after", ""},
        {"showSeconds", false},
        {"twentyfourhr", false},
        {"alignRight", true},
        {"bold", true},
        {"nerd-icon", ""},
        {"background", {128, 128, 128}}
    }},
    {"user", {
        {"enabled", true},
        {"before", ""},
        {"background", {125, 125, 213}},
        {"after", ""},
        {"nerd-icon", ""},
        {"bold", false}
    }},
    {"group", {
        {"enabled", false},
        {"before", ""},
        {"foreground", {0, 0, 0}},
        {"background", {129, 212, 9}},
        {"after", ""},
        {"bold", false}
    }},
    {"hostname", {
        {"enabled", false},
        {"before", ""},
        {"background", {144, 238, 144}},
        {"foreground", {65, 65, 65}},
        {"after", " "},
        {"nerd-icon", ""},
        {"bold", false}
    }},
    {"git-branch", {
        {"enabled", true},
        {"before", ""},
        {"foreground", {65, 65, 65}},
        {"background", {255, 165, 0}},
        {"after", ""},
        {"nerd-icon", ""},
        {"bold", false}
    }},
    {"currentdir", {
        {"enabled", true},
        {"before", ""},
        {"foreground", {65, 65, 65}},
        {"background", {255, 255, 0}},
        {"after", ""},
        {"nerd-icon", ""},
        {"bold", false},
        {"separator", "/"},
        {"shorten", true},
        {"homechar", "~"}
    }},
    {"jobs", {
        {"enabled", true},
        {"before", ""},
        {"foreground", {65, 65, 65}},
        {"background", {242, 112, 89}},
        {"after", ""},
        {"nerd-icon", ""},
        {"bold", false}
    }},
    {"ssh", {
        {"enabled", true},
        {"before", ""},
        {"background", {138, 201, 38}},
        {"after", ""},
        {"text", "ssh"},
        {"nerd-icon", ""},
        {"bold", false}
    }},
    {"prompt", {
        {"enabled", true},
        {"before", "└─"},
        {"foreground", {173, 181, 189}},
        {"after", ""},
        {"newlineBefore", true},
        {"newlineAfter", false},
        {"character", "λ "},
        {"bold", false}
    }},
    {"order", {
        "before-all", "time", "user", "group", "hostname",
        "currentdir", "git-branch", "jobs", "ssh"
    }}
};

    return j.dump(2);
}

std::string get_sh_default_json() {
  nlohmann::json j = {
      {"command", {
          {"foreground", {96, 213, 200}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"subcommand", {
          {"foreground", {226, 149, 120}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"flags", {
          {"foreground", {131, 197, 190}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"quotes", {
          {"foreground", {88, 129, 87}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"quotes_pref", {
          {"foreground", {221, 161, 94}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"exec_flags", {
          {"foreground", {255, 0, 255}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"comments", {
          {"foreground", {73, 80, 87}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"paths", {
          {"foreground", {221, 161, 94}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"operators", {
          {"foreground", {96, 213, 200}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"links", {
          {"foreground", {17, 138, 178}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", true}
      }},
      {"number", {
          {"foreground", {52, 160, 164}},
          {"background", {256, 256, 256}},
          {"bold", false},
          {"italic", false},
          {"underline", false}
      }},
      {"vars", {
          {"foreground", {244, 151, 142}},
          {"background", {256, 256, 256}}
      }}
  };

  return j.dump(2);
}

//////////////

int install() {
  std::stringstream ss;
  ss << yellow <<
  " .--.--.     ,--,                            ,---,     \n"
  " /  /    '. ,--.'|                          ,--.' |     \n"
  "|  :  /`. / |  | :                          |  |  :     \n"
  ";  |  |--`  :  : '                 .--.--.  :  :  :     \n"
  "|  :  ;_    |  ' |     ,--.--.    /  /    ' :  |  |,--. \n"
  " \\  \\    `. '  | |    /       \\  |  :  /`./ |  :  '   | \n"
  "  `----.   \\|  | :   .--.  .-. | |  :  ;_   |  |   /' : \n"
  "  __ \\  \\  |'  : |__  \\__\\/: . .  \\  \\    `.'  :  | | | \n"
  " /  /`--'  /|  | '.'| ,\" .--.; |   `----.   \\  |  ' | : \n"
  "'--'.     / ;  :    ;/  /  ,.  |  /  /`--'  /  :  :_:,\' \n"
  "  `--'---'  |  ,   /;  :   .'   \\'--'.     /|  | ,'     \n"
  "             ---`-' |  ,     .-./  `--'---' `--''       \n"
  "                     `--`---'                           \n"
  << "\x1b[0m";

  print(ss.str());
  print(green + "Welcome to the slash installer! Thank you for choosing slash!\n" + reset);
  print("Before installation, you must first confirm:\n");
  print("1. You have the entire slash and slash-utils source code in src/\n");
  print("2. The repository structure is unmodified\n");
  print("3. You are running as sudo (to move the slash binary to /usr/local/bin)\n");
  print("3. You have the following dependencies installed:\n");
  print(cyan + " cmake  g++ | libgit2  boost-regex  openssl nlohmann::json" + reset + "\n");
  print("If you are curious:\n - cmake and g++ are used for building and compilation\n - libgit2 is for git support in the prompt, and also for read and ls\n - boost-regex is used for syntax highlighting\n - ftxui is used to make TUI apps like pager\n - openssl is simply used to calculate SHA-256 and MD5 in the sumcheck command\n");
  print("Do you meet all the following conditions? (Y/N): ");

  char buffer[2];
  ssize_t bytesRead = read(STDIN_FILENO, buffer, 1);
  buffer[bytesRead] = '\0';

  if(tolower(buffer[0]) == 'n') return 0;
  print("Great! Installation will start now.\n\n");

  std::string HOME = getenv("HOME");
  if(getenv("HOME") == nullptr) {
    error("Please specify a home directory");
    return -1;
  }
  print("[Setup] Creating .slash directory in ~/\n");
    if (create_file(HOME + "/.slash", true) != 0) return 1;
    std::string slash_path = HOME + "/.slash";

    print("[Setup] Creating .slash_history\n");
    if (create_file(slash_path + "/.slash_history", false) != 0) return 1;

    print("[Setup] Creating .slash_aliases\n");
    if (create_file(slash_path + "/.slash_aliases", false) != 0) return 1;

    print("[Setup] Creating .slash_variables\n");
    if (create_file(slash_path + "/.slash_variables", false) != 0) return 1;

    print("[Setup] Creating .slashrc\n");
    if (create_file(slash_path + "/.slashrc", false) != 0) return 1;

    if (overwrite_file(slash_path + "/.slashrc", "slash-greeting") != 0) return 1;

    print("[Setup] Creating ~/.slash/config\n");
    if (create_file(slash_path + "/config", true) != 0) return 1;

    print("[Setup] Creating settings.json\n");
    if (create_file(slash_path + "/config/settings.json", false) != 0) return 1;

    print("[Setup] Writing to settings.json\n");
    if (overwrite_file(slash_path + "/config/settings.json", get_settings()) != 0) return 1;

    print("[Setup] Creating ~/.slash/config/prompts\n");
    if (create_file(slash_path + "/config/prompts", true) != 0) return 1;

    print("[Setup] Creating ~/.slash/config/prompts/default.json\n");
    if (create_file(slash_path + "/config/prompts/default.json", false) != 0) return 1;

    print("[Setup] Creating syntax-highlighting-themes/\n");
    if (create_file(slash_path + "/config/syntax-highlighting-themes", true) != 0) return 1;

    print("[Setup] Creating syntax-highlighting-themes/default.json\n");
    if(create_file(slash_path + "/config/syntax-highlighting-themes/default.json", false) != 0) return 1;

    print("[Setup] Writing to default.json\n");
    if(overwrite_file(slash_path + "/config/syntax-highlighting-themes/default.json", get_sh_default_json()) != 0) return 1;

    print("[Setup] Writing to default.json\n");
    if(overwrite_file(slash_path + "/config/prompts/default.json", get_default_json_content()) != 0) return 1;

    print("[Setup] Creating ~/.slash/slash-utils\n");
    if (create_file(slash_path + "/slash-utils", true) != 0) return 1;

    print("[Compilation] Creating temporary 'build' directory\n");
    if (create_file("build", true) != 0) return 1;

    if (chdir("build") != 0) {
      error("Failed to enter build directory.");
      return 1;
    }

    print("[Compilation] Setting up CMake\n");
    if (execute("cmake ..") != 0) return 1;

    print("[Compilation] Building slash...\n");
    if (execute("cmake --build .") != 0) return 1;

    if (chdir("../src/slash-utils") != 0) {
      error("Failed to enter src/slash-utils directory: ", true);
      return 1;
    }

    if(getenv("HOME") == nullptr) {
      error("Please set a HOME variable\n");
      return -1;
    }
    std::string home = getenv("HOME");

    print("[slash-utils] Creating shared library libslashutils\n");
    system("g++ -std=c++20 -fPIC -shared "
       "../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp "
       "../cmd_highlighter.cpp ../abstractions/json.cpp ../tui/tui.cpp ../git/git.cpp "
       "-o ~/.slash/slash-utils/libslashutils.so "
       "-lgit2 -lssl -lcrypto");

    const char* ld_path = getenv("LD_LIBRARY_PATH");
    std::string new_ld_path = (ld_path ? std::string(ld_path) : "") + ":" + home + "/.slash/slash-utils";
    setenv("LD_LIBRARY_PATH", new_ld_path.c_str(), 1);

    print("[Compilation] Building all slash-utils. This might take a minute..");

    std::vector<std::string> commands = {
        "g++ -std=c++20 acart.cpp -o " + home + "/.slash/slash-utils/acart -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 ansi.cpp -o " + home + "/.slash/slash-utils/ansi -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 cmsh.cpp -o " + home + "/.slash/slash-utils/cmsh -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 datetime.cpp -o " + home + "/.slash/slash-utils/datetime -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 dump.cpp -o " + home + "/.slash/slash-utils/dump -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 eol.cpp -o " + home + "/.slash/slash-utils/eol -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 listen.cpp -o " + home + "/.slash/slash-utils/listen -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 move.cpp -o " + home + "/.slash/slash-utils/move -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 perms.cpp -o " + home + "/.slash/slash-utils/perms -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 ascii.cpp -o " + home + "/.slash/slash-utils/ascii -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 create.cpp -o " + home + "/.slash/slash-utils/create -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 del.cpp -o " + home + "/.slash/slash-utils/del -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 echo.cpp -o " + home + "/.slash/slash-utils/echo -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 fnd.cpp -o " + home + "/.slash/slash-utils/fnd -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 ls.cpp -o " + home + "/.slash/slash-utils/ls -L" + home + "/.slash/slash-utils -lslashutils -lgit2",
        "g++ -std=c++20 clear.cpp -o " + home + "/.slash/slash-utils/clear -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 csv.cpp -o " + home + "/.slash/slash-utils/csv -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 disku.cpp -o " + home + "/.slash/slash-utils/disku -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 encode.cpp -o " + home + "/.slash/slash-utils/encode -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 mkdir.cpp -o " + home + "/.slash/slash-utils/mkdir -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 pager.cpp -o " + home + "/.slash/slash-utils/pager -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 sumcheck.cpp -o " + home + "/.slash/slash-utils/sumcheck -L" + home + "/.slash/slash-utils -lslashutils -lssl -lcrypto",
        "g++ -std=c++20 textmt.cpp -o " + home + "/.slash/slash-utils/textmt -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 netinfo.cpp -o " + home + "/.slash/slash-utils/netinfo -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 ren.cpp -o " + home + "/.slash/slash-utils/ren -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 lynx.cpp -o " + home + "/.slash/slash-utils/lynx -L" + home + "/.slash/slash-utils -lslashutils -lgit2",
        "g++ -std=c++20 srch.cpp -o " + home + "/.slash/slash-utils/srch -L" + home + "/.slash/slash-utils -lslashutils",
        "g++ -std=c++20 md.cpp -o " + home + "/.slash/slash-utils/md -L" + home + "/.slash/slash-utils -lslashutils"
    };


  for(int i = 0; i < commands.size(); i++) {
    system(commands[i].c_str());
  }

  print("\x1b[7mPLEASE RUN THE FOLLOWING COMMAND UNDER ROOT\x1b[0m\n");
  print("sudo mv build/slash /usr/local/bin/slash\n\n");

  print("This is not a slash limitation, but a system limitation.\n");
  print("Running the installer under root will only make slash usable for the root,\n");
  print("just like installing slash will only make it usable for the current user\n");
  return 0;
}

int main() {
  return install();
}
