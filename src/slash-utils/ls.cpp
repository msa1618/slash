#include "../command.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <math.h>

class Ls : public Command {
	private:
		int default_print(std::string dir_path, bool print_hidden) {
			char buffer[512];
			const char* path;

			if (dir_path.empty()) {
				path = getcwd(buffer, 512);
			} else {
				path = dir_path.c_str();
			}

			DIR *d = opendir(path);
			if(!d) {
				perror("ls");
				return -1;
			}

			struct dirent *entry;
			int longest_name_length = 0;

			std::vector<std::pair<std::string, std::string>> entries;

			while ((entry = readdir(d)) != NULL) {
				std::string name = entry->d_name;
				if ((name == "." || name == "..") || (!print_hidden && name.starts_with("."))) {
					continue;
				}

				char *type;
				switch (entry->d_type) {
					case DT_REG:
						type = "file";
						break;
					case DT_DIR:
						type = "dir";
						break;
					case DT_LNK:
						type = "link";
						break;
					default:
						type = "other";
						break;
				}

				if (strlen(entry->d_name) > longest_name_length) {
					longest_name_length = strlen(entry->d_name);
				}

				entries.emplace_back(entry->d_name, type);
			}

			std::sort(entries.begin(), entries.end());

			int longest_num_width = 0;
			for(int i = 0; i < entries.size(); i++) {
				int i_str_l = std::to_string(i + 1).length();
				if(i_str_l > longest_num_width) {
					longest_num_width = i_str_l;
				}
			}

			for (int i = 0; i < entries.size(); i++) {
				std::string num = std::to_string(i + 1);
				std::string name = entries[i].first;
				std::string type = entries[i].second;

				num.resize(longest_num_width, ' ');
				name.resize(longest_name_length, ' ');
				type.resize(6, ' ');

				io::print(num);
				io::print(" | ");
				io::print(name);
				io::print(" | ");
				io::print(type);
				io::print("| \n");

			}

			closedir(d);
			return 0;
		}

	int tree_print(std::string dir_path, bool print_hidden, std::vector<bool> last_entry_stack = {}) {
		DIR *dir = opendir(dir_path.c_str());
		if (!dir) {
			info::error(strerror(errno), errno, dir_path);
			return -1;
		}

		struct dirent *entry;
		std::vector<std::string> names;

		// Collect entries
		while ((entry = readdir(dir)) != nullptr) {
			std::string name = entry->d_name;
			if ((name == "." || name == "..") || (!print_hidden && name.starts_with("."))) {
				continue;
			}
			names.push_back(name);
		}

		std::sort(names.begin(), names.end());
		for (size_t i = 0; i < names.size(); ++i) {
			bool is_last = (i == names.size() - 1);
			std::string name = names[i];
			std::string path = dir_path + "/" + name;

			struct stat path_stat;
			if (lstat(path.c_str(), &path_stat) == -1) {
				perror("lstat");
				continue;
			}

			// Print indentation and vertical lines for ancestor levels
			for (size_t lvl = 0; lvl < last_entry_stack.size(); ++lvl) {
				if (last_entry_stack[lvl]) {
					io::print("  ");    // no vertical line, last entry at this level
				} else {
					io::print("│ ");
				}
			}

			// Print branch for current entry
			io::print(is_last ? "└─" : "├─");

			if (S_ISDIR(path_stat.st_mode)) {
				io::print(blue + name + reset + "\n");
				auto new_stack = last_entry_stack;
				new_stack.push_back(is_last);
				tree_print(path, print_hidden, new_stack);
			} else if (S_ISLNK(path_stat.st_mode)) {
				char buf[1024];
				ssize_t len = readlink(path.c_str(), buf, sizeof(buf) - 1);
				if (len != -1) {
					buf[len] = '\0';
					io::print(orange + name + reset + " -> " + buf + "\n");
				} else {
					io::print(orange + name + reset + " -> [unreadable]\n");
				}
			} else {
				io::print(name + "\n");
			}
		}

		closedir(dir);
		return 0;
	}

	public:
		Ls() : Command("ls", "Lists all files, directories, and links in a directory", "") {}

		int exec(std::vector<std::string> args) {
			if(args.empty()) {
				char buffer[1024];
				getcwd(buffer, 1024);

				default_print(buffer, false);
				return 0;
			}

			std::string path;
			bool print_tree = false;
			bool print_hidden = false;

			std::vector<std::string> validArgs = {
				"-t", "-a",
				"--tree", "--all"
			};

			for(auto& arg : args) {
				if(!io::vecContains(validArgs, arg) && arg.starts_with('-')) {
					info::error(std::string("Bad argument: ") + arg);
					return -1;
				}

				if(arg == "-t" || arg == "--tree") print_tree = true;
				if(arg == "-a" || arg == "--all") print_hidden = true;

				if(!arg.starts_with("-")) {
					path = arg;
				}
			}

			std::string dirpath = path;
			if(path.empty()) {
				char buffer[1024];
				getcwd(buffer, 1024);
				dirpath = buffer;
			}

			std::string s = print_hidden ? "true" : "false";
			info::debug("ph " + s);

			if(print_tree) {
				tree_print(dirpath, print_hidden);
				return 0;
			} else {
				default_print(dirpath, print_hidden);
				return 0;
			}
		}
};

int main(int argc, char* argv[]) {
	Ls ls;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	ls.exec(args);
	return 0;
}
