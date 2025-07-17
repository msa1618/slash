#include "cd.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "../abstractions/definitions.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#pragma region helpers

#pragma endregion

void create_alias(std::string name, std::string dirpath) {
	std::string aliases_path = slash_dir + "/.cd_aliases";
	std::string line = "@" + name + " = " + dirpath + "\n";
	if(io::write_to_file(aliases_path, line) != 0) {
		std::string err = std::string("Failed to write to .cd_aliases: ") + strerror(errno);
		info::error(err, errno, ".cd_aliases");
	}
}

std::variant<std::string, int> get_alias(std::string alias) {
	auto contents = io::read_file(slash_dir + "/.cd_aliases");
	if(std::holds_alternative<int>(contents)) {
		std::string err = std::string("Failed to read .cd_aliases: ") + strerror(errno);
		info::error(err, errno, ".cd_aliases");
		return errno;
	}
	std::vector<std::string> lines = io::split(std::get<std::string>(contents), '\n');
	for(auto& line : lines) {
		std::string prefix = "@" + alias + " = ";
		if(line.starts_with(prefix)) {
			std::string path = line.substr(prefix.length());
			return path;
		}
	}
	return -1;
}

int cd(std::vector<std::string> args) {
	if(args.empty()) {
		io::print("cd: change directory\n"
							"usage: cd <directory>\n"
							"flags:\n"
							"-c | --create-alias: creates an alias to easily switch to frequently used directories\n"
							"                     e.g. cd @work = cd ~/personal/work");
		return 0;
	}

	std::vector<std::string> valid_args = {
		"--create-alias", "-c"
	};

	bool create_alias_mode = false;

	for(auto& arg : args) {
		if(!io::vecContains(valid_args, arg) && arg.starts_with('-')) {
			info::error("Invalid argument");
			return -1;
		}

		if(arg == "--create-alias" || arg == "-c") create_alias_mode = true;
	}

	if(create_alias_mode) {
		if(args.size() > 2) {
			std::string alias_name = args[2];
			std::string path = args[3];
			if(!alias_name.empty() && !path.empty()) {
				create_alias(alias_name, path);
				return 0;
			}
		}
	}

	if(args[1].starts_with("@")) {
		auto alias = get_alias(args[1].substr(1));
		if(std::holds_alternative<std::string>(alias)) {
			if(chdir(std::get<std::string>(alias).c_str()) != 0) {
				info::error("Alias does not exist.");
				return -1;
			}
		}
		return 0;
	}

	if(chdir(args[1].c_str()) != 0) {
		info::error(strerror(errno), errno);
		return -1;
	}

	return 0;
}
