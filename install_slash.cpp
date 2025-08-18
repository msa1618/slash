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

void error(std::string error, bool print_strerror = false) {
  std::string red = "\x1b[31m";
  print(red + "[Error] " + reset + error + "\n");
  if(print_strerror) print(strerror(errno));
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

int overwrite_file(std::string path, std::string content) {
  int fd = open(path.c_str(), O_WRONLY);
  if(fd < 0) { close(fd); return errno; }

  ssize_t written = write(fd, content.c_str(), content.size());
  if(written < 0) {close(fd); return errno; };

  if(written < content.size()) { close(fd); return EIO; }
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
        {"time", {
            {"enabled", true},
            {"before", "["},
            {"after", "] "},
            {"showSeconds", false},
            {"twentyfourhr", false},
            {"alignRight", true},
            {"bold", true},
            {"color", {128, 128, 128}}
        }},
        {"user", {
            {"enabled", false},
            {"color", {173, 216, 230}},
            {"after", "|"},
            {"bold", false}
        }},
        {"group", {
            {"enabled", false},
            {"color", {100, 149, 237}},
            {"after", ""},
            {"bold", false}
        }},
        {"hostname", {
            {"enabled", false},
            {"color", {144, 238, 144}},
            {"after", " "},
            {"bold", false}
        }},
        {"git-branch", {
            {"enabled", true},
            {"color", {255, 165, 0}},
            {"after", " "},
            {"bold", ""}
        }},
        {"currentdir", {
            {"enabled", true},
            {"color", {255, 255, 0}},
            {"after", " "},
            {"bold", false}
        }},
        {"ssh", {
            {"enabled", true},
            {"color", {255, 105, 180}},
            {"after", " "},
            {"text", "ssh"},
            {"bold", false}
        }},
        {"prompt", {
            {"enabled", true},
            {"color", {173, 181, 189}},
            {"after", " "},
            {"newlineBefore", true},
            {"newlineAfter", false},
            {"character", "$"},
            {"bold", false}
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
  print("[Setup] Creating .slash directory in ~/\n");
    if (create_file(HOME + "/.slash", true) != 0) return 1;
    std::string slash_path = HOME + "/.slash";

    print("[Setup] Creating .slash_history\n");
    if (create_file(slash_path + "/.slash_history", false) != 0) return 1;

    print("[Setup] Creating .slash_aliases\n");
    if (create_file(slash_path + "/.slash_aliases", false) != 0) return 1;

    print("[Setup] Creating .slash_variables\n");
    if (create_file(slash_path + "/.slash_variables", false) != 0) return 1;

    print("[Setup] Creating .slash_startup_commands\n");
    if (create_file(slash_path + "/.slash_startup_commands", false) != 0) return 1;

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

    print("[Compilation] Moving slash to usr/local/bin\n");
    std::string lbpath = HOME + "usr/local/bin/slash";
    if (rename("slash", lbpath.c_str()) != 0) {
      error("Failed to move slash to usr/local/bin: ", true);
      return 1;
    }

    if (chdir("../src/slash-utils") != 0) {
      error("Failed to enter src/slash-utils directory.");
      return 1;
    }

    print("[slash-utils] Compiling all slash utilities. This might take a while..\n");
    if (create_file("/tmp/slash-utils", true) != 0) return 1;
  
    std::vector<std::string> commands = {
      "g++ -std=c++20 acart.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/acart -lboost_regex",
      "g++ -std=c++20 ansi.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/ansi -lboost_regex",
      "g++ -std=c++20 cmsh.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/cmsh -lboost_regex",
      "g++ -std=c++20 datetime.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/datetime -lboost_regex",
      "g++ -std=c++20 dump.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/dump -lboost_regex",
      "g++ -std=c++20 eol.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/eol -lboost_regex",
      "g++ -std=c++20 listen.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/listen -lboost_regex",
      "g++ -std=c++20 move.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/move -lboost_regex",
      "g++ -std=c++20 perms.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/perms -lboost_regex",
      "g++ -std=c++20 read.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../git/git.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/read -lgit2 -lboost_regex",
      "g++ -std=c++20 ascii.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/ascii -lboost_regex",
      "g++ -std=c++20 create.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/create -lboost_regex",
      "g++ -std=c++20 del.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/del -lboost_regex",
      "g++ -std=c++20 echo.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/echo -lboost_regex",
      "g++ -std=c++20 fnd.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/fnd -lboost_regex",
      "g++ -std=c++20 ls.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../git/git.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/ls -lgit2 -lboost_regex",
      "g++ -std=c++20 clear.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/clear -lboost_regex",
      "g++ -std=c++20 csv.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/csv -lboost_regex",
      "g++ -std=c++20 disku.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/disku -lboost_regex",
      "g++ -std=c++20 encode.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/encode -lboost_regex",
      "g++ -std=c++20 mkdir.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/mkdir -lboost_regex",
      "g++ -std=c++20 pager.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/pager -lboost_regex",
      "g++ -std=c++20 sumcheck.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/sumcheck -lssl -lcrypto -lboost_regex",
      "g++ -std=c++20 textmt.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/textmt -lboost_regex",
      "g++ -std=c++20 netinfo.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/netinfo -lboost_regex",
      "g++ -std=c++20 ren.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/ren -lboost_regex",
      "g++ -std=c++20 lynx.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/lynx -lgit2 -lboost_regex",
      "g++ -std=c++20 mkf.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/mkf -lboost_regex",
      "g++ -std=c++20 srch.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/srch -lboost_regex",
      "g++ -std=c++20 md.cpp ../abstractions/iofuncs.cpp ../abstractions/info.cpp ../help_helper.cpp ../cmd_highlighter.cpp -o $HOME/.slash/slash-utils/md -lboost_regex"
  };


  for(int i = 0; i < commands.size(); i++) {
    system(commands[i].c_str());
  }

  print(green + "Slash has been installed!" + reset + "\n");
  return 0;
}

int main() {
  return install();
}
