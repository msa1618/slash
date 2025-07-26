#include "alias.h"

#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../command.h"

#include <unistd.h>
#include <algorithm>
#include <string>
#include <vector>

void list_aliases() {
	std::string home = getenv("HOME");
	std::string aliases_path = home + "/.slash/.slash_aliases";

	auto content = io::read_file(aliases_path);
	if(!std::holds_alternative<std::string>(content)) {
		std::string error = std::string("Failed to read \"" + aliases_path + "\": ") + strerror(errno);
		info::error(error, errno);
		return;
	}

	std::vector<std::string> aliases = io::split(std::get<std::string>(content), "\n");
	aliases.erase(std::remove_if(aliases.begin(), aliases.end(), [](std::string& a){
		return a == "\n" || a.starts_with("//");
	}), aliases.end());
  if(aliases.empty()) {
    io::print("No aliases found\n");
    return;
  }

	int longest_alias_length = 0;
	for(auto& a : aliases) {
    if(a == "\n" || a.starts_with("//")) continue; // Removed but just in case
		if(a.length() > longest_alias_length) longest_alias_length = a.length();
	}

	for(int i = 0; i < aliases.size(); i++) {
		std::vector<std::string> value = io::split(aliases[i], " = ");
		if(value.size() != 2) {
			continue; // Invalid alias format
		}
		value[0].resize(longest_alias_length, ' ');
		io::print(yellow + value[0] + reset + " = " + value[1] + "\n");
	}
}

void create_alias(const std::string& name, const std::string& value) {
	std::string home = getenv("HOME");
	std::string aliases_path = home + "/.slash/.slash_aliases";

	auto content = io::read_file(aliases_path);
	if(!std::holds_alternative<std::string>(content)) {
		std::string error = std::string("Failed to read \"" + aliases_path + "\": ") + strerror(errno);
		info::error(error, errno);
		return;
	}

	std::vector<std::string> aliases = io::split(std::get<std::string>(content), "\n");
	aliases.erase(std::remove_if(aliases.begin(), aliases.end(), [](std::string& a){
		return a == "\n" || a.starts_with("//");
	}), aliases.end());

	for(auto& a : aliases) {
		if(a == name + " = " + value) {
			info::error("Alias \"" + name + "\" already exists\n");
			return;
		}
	}

	if(std::get<std::string>(content).empty()) {
    aliases.push_back(name + " = " + value);
  } else {
    aliases.push_back("\n" + name + " = " + value);
  }
	io::overwrite_file(aliases_path, io::join(aliases, "\n"));
}

std::string get_alias(std::string name, bool print) {
  std::string home = getenv("HOME");
  std::string aliases_path = home + "/.slash/.slash_aliases";

  auto content = io::read_file(aliases_path);
  if(!std::holds_alternative<std::string>(content)) {
    std::string error = std::string("Failed to read \"" + aliases_path + "\": ") + strerror(errno);
    info::error(error, errno);
    return "UNKNOWN";
  }

  std::vector<std::string> aliases = io::split(std::get<std::string>(content), "\n");
  aliases.erase(std::remove_if(aliases.begin(), aliases.end(), [](std::string& a){
    return a == "\n" || a.starts_with("//");
  }), aliases.end());

  for(auto& a : aliases) {
    if(a.starts_with(name + " = ")) {
      std::string value = a.substr(name.length() + 3); // 3 for " = "
      if(print) {
        io::print(yellow + name + reset + " = " + value);
      }
      return value;
    }
  }

  if(print) {
    info::error("Alias \"" + name + "\" does not exist\n");
  }

  return "UNKNOWN";
}

void delete_alias(std::string name) {
	std::string home = getenv("HOME");
	std::string aliases_path = home + "/.slash/.slash_aliases";

	auto content = io::read_file(aliases_path);
	if(!std::holds_alternative<std::string>(content)) {
		std::string error = std::string("Failed to read \"" + aliases_path + "\": ") + strerror(errno);
		info::error(error, errno);
		return;
	}

	std::vector<std::string> aliases = io::split(std::get<std::string>(content), "\n");
	aliases.erase(std::remove_if(aliases.begin(), aliases.end(), [](std::string& a){
		return a == "\n" || a.starts_with("//");
	}), aliases.end());

	auto it = std::remove_if(aliases.begin(), aliases.end(), [&](const std::string& a) {
		return a.starts_with(name + " = ");
	});

	if(it == aliases.end()) {
		info::error("Alias \"" + name + "\" does not exist\n");
		return;
	}

	aliases.erase(it, aliases.end());
	io::write_to_file(aliases_path, io::join(aliases, "\n"));
}

void delete_all_aliases() {
  std::string home = getenv("HOME");
  std::string aliases_path = home + "/.slash/.slash_aliases";

  int aliases_num = [aliases_path](){
    auto content = io::read_file(aliases_path);
    if(!std::holds_alternative<std::string>(content)) {
      std::string error = std::string("Failed to read \"" + aliases_path + "\": ") + strerror(errno);
      info::error(error, errno);
      return -1;
    }
    std::vector<std::string> aliases = io::split(std::get<std::string>(content), "\n");
    aliases.erase(std::remove_if(aliases.begin(), aliases.end(), [](std::string& a){
      return a == "\n" || a.starts_with("//");
    }), aliases.end());
    return (int)aliases.size();
  }();

  if(aliases_num == -1) return;
  if(aliases_num == 0) {
    io::print("No aliases to delete.\n");
    return;
  }

  char choice[2];
  info::warning("Are you sure you want to delete " + std::to_string(aliases_num) + " aliases? (Y/N): ");

  ssize_t bytesRead = read(STDIN_FILENO, choice, 1);
  if(bytesRead < 0) {
    std::string error = std::string("Failed to get input: ") + strerror(errno);
    info::error(error, errno);
    return;
  }
  choice[bytesRead] = '\0';

  if(choice[0] == 'n' || choice[0] == 'N') {
    return;
  }

  if(io::overwrite_file(aliases_path, "") != 0) {
    std::string error = std::string("Failed to clear aliases: ") + strerror(errno);
    info::error(error, errno, aliases_path);
  }
}

int alias(std::vector<std::string> args) {
	if(args.empty()) {
		io::print("alias: manipulate command aliases\n");
		io::print("usage: alias [name] [value]\n");
		io::print("flags:\n");
		io::print("  -l | --list:       list all aliases\n");
		io::print("  -d | --delete:     delete an alias\n");
    io::print("  -D | --delete-all: delete all aliases\n");
    io::print("  -g | --get:        get an alias's value\n");
		io::print("  -c | --create:     create new alias\n");
		return 0;
	}

	std::vector<std::string> validArgs = {
		"-l", "--list",
		"-d", "--delete",
    "-D", "--delete-all",
		"-g", "--get",
		"-c", "--create"
	};

	for(auto& arg : args) {
		if(!io::vecContains(validArgs, arg) && !arg.starts_with('-')) {
			info::error(std::string("Invalid argument \"") + arg + "\"");
			return -1;
		}

		if(arg == "-l" || arg == "--list") {
			list_aliases();
			return 0;
		}

		if(arg == "-d" || arg == "--delete") {
			if(args.size() < 2) {
				info::error("No alias name provided for deletion");
				return -1;
			}
			delete_alias(args[1]);
			return 0;
		}

    if(arg == "-D" || arg == "--delete-all") {
      delete_all_aliases();
      return 0;
    }

    if(arg == "-g" || arg == "--get") {
      if(args.size() < 2) {
        info::error("No alias name provided");
        return -1;
      }
      get_alias(args[1], true);
      return 0;
    }

		if(arg == "-c" || arg == "--create") {
			if(args.size() < 3) {
				info::error("No alias name or value provided");
				return -1;
			}
			create_alias(args[1], args[2]);
			return 0;
		}
	}
	return 0;
}
