#include "cd.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <algorithm>
#include "../abstractions/definitions.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

/*
#pragma region helpers

std::variant<std::string, int> get_cd_alias(std::string alias) {
  auto contents = io::read_file(slash_dir + "/.cd_aliases");
  if(std::holds_alternative<int>(contents)) {
    std::string err = std::string("Failed to read .cd_aliases: ") + strerror(errno);
    info::error(err, errno, ".cd_aliases");
    return errno;
  }
  std::vector<std::string> lines = io::split(std::get<std::string>(contents), "\n");
  for(auto& line : lines) {
    std::string prefix = "@" + alias + " = ";
    if(line.starts_with(prefix)) {
      std::string path = line.substr(prefix.length());
      return path;
    }
  }
  return -1;
}

int create_alias(std::string name, std::string dirpath) {
  if(name.starts_with("@")) name = name.substr(1);

  std::string home_dir = getenv("HOME");
  std::string aliases_path = home_dir + "/.slash/.cd_aliases";
  std::string line = "@" + name + " = " + dirpath + "\n";

  //////////

  auto existing = get_cd_alias(name);
  bool aliasExists = std::holds_alternative<std::string>(existing);

  if(aliasExists) {
    info::warning("The alias already exists. Do you want to overwrite it?  (Y/N): ");

    char buffer[2];
    ssize_t bytesRead = read(STDIN, buffer, 1);
    if(bytesRead < 0) {
      std::string error = std::string("Failed to get input") + strerror(errno);
      info::error(error, errno);
      return -1;
    }

    buffer[bytesRead] = '\0';
    if(buffer[0] == 'n' || buffer[0] == 'N') return 0;
    if(buffer[0] == 'y' || buffer[0] == 'Y') {
      auto content = io::read_file(aliases_path);
      if(!std::holds_alternative<std::string>(content)) {
        std::string error = std::string("Failed to read .cd_aliases: ") + strerror(errno);
        info::error(error, errno);
        return -1;
      }
      std::vector<std::string> lines = io::split(std::get<std::string>(content), "\n");
      lines.erase(std::remove_if(lines.begin(), lines.end(), [&](const std::string& line) {
        return line.starts_with("@" + name + " = ");
      }), lines.end());
      lines.push_back(line);

      if(io::overwrite_file(aliases_path, io::join(lines, "\n")) != 0) {
        std::string error = std::string("Failed to read .cd_aliases: ") + strerror(errno);
        info::error(error, errno);
        return -1;
      }
    }

    return 0;
  }

  //////////

  if(io::write_to_file(aliases_path, line) != 0) {
    std::string err = std::string("Failed to write to .cd_aliases: ") + strerror(errno);
    info::error(err, errno, ".cd_aliases");
  }
}

void list_cd_aliases() {
  std::string home = getenv("HOME");
  auto aliases = io::read_file(home + "/.slash/.cd_aliases");
  if(!std::holds_alternative<std::string>(aliases)) {
    std::string error = std::string("Failed to read .cd_aliases") + strerror(errno);
    info::error(error, errno);
    return;
  }

  int longest_alias_name = 0;
  std::vector<std::string> result;

  std::string content = std::get<std::string>(aliases);
  std::vector<std::string> lines = io::split(content, "\n");
  if(lines.empty()) return;

  for(auto& l : lines) { // First loop: Get the longest alias name
    if(l.starts_with("//")) continue; // Comment
    if(l.empty() || l == "\n") continue; // A whole line with just \n will cause a segfault
    auto pair = io::split(l, "=");
    if(pair[0].length() > longest_alias_name) longest_alias_name = pair[0].length();
  }

  for(auto& line : lines) {
    if(line.starts_with("//")) continue;
    if(line.empty()) continue;
    auto pair = io::split(line, "=");
    pair[0].resize(longest_alias_name, ' ');

    std::stringstream output;
    output << blue << pair[0] << reset << " = " << pair[1] << "\n";
    io::print(output.str());
  }
}

#pragma endregion

*/

int cd(std::vector<std::string> args) {
  for(auto& arg : args)
  if(args.size() > 2) {
    io::print("cd: change directory\n"
              "usage: cd <directory>\n"
              "flags:\n"
              "-c | --create-alias: creates an alias to easily switch to frequently used directories\n"
              "                     e.g. cd @work = cd ~/personal/work");
    return 0;
  }

  std::vector<std::string> valid_args = {
    "--create-alias", "-c", "-la", "--list-aliases"
  };

  bool create_alias_mode = false;

  for(auto& arg : args) {
    if(!io::vecContains(valid_args, arg) && arg.starts_with('-')) {
      info::error("Invalid argument");
      return EINVAL;
    }

    if(arg == "--create-alias" || arg == "-c") create_alias_mode = true;
    if(arg == "--list-aliases" || arg == "-la") {
      //l//ist_cd_aliases();
      return 0;
    }
  }

  if(create_alias_mode) {
    if(args.size() > 2) {
      std::string alias_name = args[2];
      std::string path = args[3];
      if(!alias_name.empty() && !path.empty()) {
        //create_alias(alias_name, path);
        return 0;
      }
    }
  }

  if(args.size() > 2) {
    /*
    if(args[1].starts_with("@")) {
    auto alias = get_cd_alias(args[1].substr(1));
    if(std::holds_alternative<std::string>(alias)) {
      if(chdir(std::get<std::string>(alias).c_str()) != 0) {
        info::error("Failed to change directory to alias path");
        return -1;
      }
      return 0;
    }
    

    if(std::holds_alternative<int>(alias)) {
      int code = std::get<int>(alias);
      if(code == -1) { // Alias not found
        info::error("Alias not found");
        return -1;
      }
    }
      */
  }



  if(args[1].starts_with("~")) {
    std::string home = getenv("HOME");
    io::replace_all(args[1], "~", home);
  }
  if(chdir(args[1].c_str()) != 0) {
    info::error(strerror(errno), errno);
    return -1;
  }

  return 0;
}
