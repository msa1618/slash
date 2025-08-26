#include "var.h"
#include <unistd.h>
#include <filesystem>
#include <algorithm>
#include "../help_helper.h"
#include "../cmd_highlighter.h"

std::vector<Variable> temp_vars;

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
  for(auto& var : temp_vars) {
    if(var.name.length() > longest_var_length) longest_var_length = var.name.length();
  }

  if(!temp_vars.empty()) {
    io::print(magenta + bold + "Temporary variables\n" + reset);
    for(auto& v : temp_vars) {
      v.name.resize(longest_var_length, ' ');
      io::print("  " + highl(v.name) + " = " + v.value + "\n");
    }
    io::print("\n");
  }

  if(!vars.empty()) {
    io::print(green + bold + "Saved variables\n" + reset);
    for(int i = 0; i < vars.size(); i++) {
      std::vector<std::string> var_and_value = io::split(vars[i], " = ");
      if(var_and_value.size() != 2) {
        info::error("Wrong line syntax on line " + std::to_string(i + 1));
        continue;
      }

      var_and_value[0].resize(longest_var_length, ' ');
      io::print(highl(var_and_value[0]) + "= " + var_and_value[1] + "\n");
    }
  }
}

void create_temp_var(std::string name, std::string value) {
  temp_vars.push_back({name, value});
}

void create_variable(std::string name, std::string value) {
  if(name.find(" ") != std::string::npos) {
    info::error("Variable cannot contain spaces!");
    return;
  }
  std::string line = "$" + name + " = \"" + value + "\"";

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
    if (l.starts_with("$" + name + " = ")) {
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
        {"-t", "--temp", "Create temporary variable, discarded after slash exit"},
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
    "-l", "-c", "-g", "-t",
    "--list", "--create", "--get", "--temp"
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
        info::error("Unspecified name or value");
        return EINVAL;
      }
      create_variable(args[i + 1], args[i + 2]);
      return 0;
    }

    if(args[i] == "-t" || args[i] == "--temp") {
      if(!(i + 1 < args.size() && i + 2 < args.size())) {
        info::error("Unspecified name or value");
        return EINVAL;
      }
      create_temp_var(args[i + 1], args[i + 2]);
      return 0;
    }

    if(args[i] == "-g" || args[i] == "--get") {
      if(i + 1 < args.size()) {
        auto name = get_value(args[i + 1]);
        if(std::holds_alternative<std::string>(name)) {
          io::print(std::get<std::string>(name) + "\n");
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
