#include "var.h"
#include <unistd.h>
#include <filesystem>
#include <algorithm>
#include "../help_helper.h"

void list_variables() {
  std::string home = getenv("HOME");
  auto content = io::read_file(home + "/.slash/.slash_variables");
  if(!std::holds_alternative<std::string>(content)) {
    std::string error = std::string("Couldn't open .slash_variables: ") + strerror(errno);
    info::error("Couldn't open .slash_variables: ", errno);
    return;
  }

  std::vector<std::string> vars = io::split(std::get<std::string>(content), "\n");
  vars.erase(std::remove_if(vars.begin(), vars.end(), [](std::string line){
    return line.empty() || line == "\n" || line.starts_with("//");
  }), vars.end());

  int longest_var_length = 0;
  for(auto& var : vars) {
    if(var.length() > longest_var_length) longest_var_length = var.length();
  }

  for(int i = 0; i < vars.size(); i++) {
    std::vector<std::string> var_and_value = io::split(vars[i], " = ");
    if(var_and_value.size() != 2) {
      info::error("Wrong line syntax on line " + std::to_string(i + 1));
      continue;
    }

    var_and_value[0].resize(longest_var_length, ' ');
    io::print(var_and_value[0] + "= " + var_and_value[1] + "\n");
  }
}

void create_variable(std::string name, std::string value) {
  if(name.find(" ") != std::string::npos) {
    info::error("Variable cannot contain spaces!");
    return;
  }
  std::string line = "$(" + name + ") = \"" + value + "\"";

  std::string home = getenv("HOME");
  std::string varfile = home + "/.slash/.slash_variables";

  auto content = io::read_file(varfile);
  if (!std::holds_alternative<std::string>(content)) {
    std::string error = std::string("Failed to open .slash_variables: ") + strerror(errno);
    info::error(error, errno);
    return;
  }

  std::vector<std::string> lines = io::split(std::get<std::string>(content), "\n");

  bool exists = false;
  for (auto &l : lines) {
    if (l.starts_with("$(" + name + ") = ")) {
      exists = true;
      break;
    }
  }

  if (exists) {
    info::warning("This variable already exists. Do you want to overwrite it? (y/n): ");
    char buffer[2];
    ssize_t bytesRead = read(STDIN_FILENO, buffer, 1);
    if (bytesRead < 0) {
      std::string error = std::string("Failed to get input") + strerror(errno);
      info::error(error, errno);
      return;
    }
    buffer[bytesRead] = '\0';

    if (buffer[0] == 'n' || buffer[0] == 'N') return;
    if (buffer[0] == 'y' || buffer[0] == 'Y') {
      lines.erase(std::remove_if(lines.begin(), lines.end(), [&](const std::string &line) {
        return line.starts_with("$(" + name + ") = ");
      }), lines.end());
    } else {
      return;
    }
  }

  lines.push_back(line);
  std::string newContent = io::join(lines, "\n");

  if (io::overwrite_file(varfile, newContent) != 0) {
    std::string error = std::string("Failed to write to .slash_variables: ") + strerror(errno);
    info::error(error, errno);
    return;
  }
}

std::variant<std::string, int> get_value(std::string name) {
  std::string home = getenv("HOME");
  std::string varfile = home + "/.slash/.slash_variables";

  auto content = io::read_file(varfile);
  if(!std::holds_alternative<std::string>(content)) {
    std::string error = std::string("Failed to open .slash_variables: ") + strerror(errno);
    info::error(error, errno);
    return -1;
  }

  std::vector<std::string> lines = io::split(std::get<std::string>(content), "\n");
  for(auto& line : lines) {
    if(line.starts_with("$(" + name + ") = ")) {
      auto var_and_value = io::split(line, " = ");
      std::string val = var_and_value[1];
      if(val.size() >= 2 && val.front() == '\"' && val.back() == '\"') {
        val = val.substr(1, val.size() - 2);
      } else {
        info::error("Variable must start with quotes. You probably edited .slash_variables manually.");
        return -1;
      }
      return val;
    }
  }

  char* env_var = getenv(name.c_str());
  if(env_var != nullptr) return env_var;

  info::error("Could not find variable " + name + ".");
  return "";
}

int var(std::vector<std::string> args) {
  if(args.empty()) {
    io::print(get_helpmsg({
      "Manipulate variables",
      {
        "var [option]",
        "var [option] <variable>"
      },
      {
        {"-l", "--list", "List all variables"},
        {"-c", "--create", "Create a variable"},
        {"-g", "--get", "Get a variable's value"}
      },
      {
        {"var -c \"work\" ~/personal/dir1/more/longer/work", "Creates variable work"},
        {"var -g \"work\"", "Get value of work"}
      }
    }));
    return 0;
  }

  std::vector<std::string> validArgs = {
    "-l", "-c", "-g",
    "--list", "--create", "--get"
  };

  for(int i = 0; i < args.size(); i++) {
    if(!io::vecContains(validArgs, args[i])) {
      info::error("Invalid argument: \"" + args[i] + "\"\n");
      return EINVAL;
    }

    if(args[i] == "-l" || args[i] == "--list") {
      list_variables();
      return 0;
    }

    if(args[i] == "-c" || args[i] == "--create") {
      if(!(i + 1 < args.size() && i + 2 < args.size())) {
        info::error("Invalid arguments!");
        return EINVAL;
      }
      create_variable(args[i + 1], args[i + 2]);
      return 0;
    }

    if(args[i] == "-g" || args[i] == "--get") {
      if(i + 1 < args.size()) {
        auto name = get_value(args[i + 1]);
        if(std::holds_alternative<std::string>(name)) {
          io::print(std::get<std::string>(name));
          return 0;
        } else {
          info::error("Could not get variable " + args[i + 1]);
          return -1;
        }
        return 0;
      } else {
        info::error("Missing variable name");
        return 0;
      }
    }
  }
  return 0;
}
