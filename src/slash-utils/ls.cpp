#include "../command.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <algorithm>
#include <math.h>

class Ls : public Command {
	private:
		int default_print(std::string dir_path) {
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

			// Added in by dirent
			entries.erase(std::remove_if(entries.begin(), entries.end(), [](const auto& e) {
				return e.first == "." || e.first == "..";
			}), entries.end());

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

		int tree_print(std::string dir_path, int indent = 2, int level = 0) {
			DIR* dir = opendir(dir_path.c_str());
			if(dir == 0) {
				info::error(strerror(errno), errno, dir_path);
				return -1;
			}

			struct dirent* entry;
			while((entry = readdir(dir)) != nullptr) {
				std::string name = entry->d_name;
				if(name == "." || name == "..") continue;

				std::stringstream fpath;
				fpath << dir_path << "/" << name;
				std::string path = fpath.str();

				for(int i = 0; i < level; i++) {
					io::print(std::string(indent, ' '));
				}

				if(entry->d_type == DT_REG) {// File
					io::print(entry->d_name);
					io::print("\n");
					continue;
				}

				if(entry->d_type == DT_DIR) {
					std::stringstream output;
					output << blue << name << reset << "\n";
					io::print(output.str());

					tree_print(path, indent, level + 1);
					continue;
				}

				if(entry->d_type == DT_LNK) {
					char buffer[1024];
					int bytesRead = readlink(path.c_str(), buffer, 1024);
					if(bytesRead == -1) {
						std::string error = std::string("Failed to read symlink target path: ") + strerror(errno);
						info::error(error, errno, path);
						continue;
					}
					buffer[bytesRead] = '\0';

					std::stringstream symlink_output;
					symlink_output << orange << name << reset << "->" << buffer << "\n";
					io::print(symlink_output.str());
				}
		}

		return 0;
}

	public:
		Ls() : Command("ls", "Lists all files, directories, and links in a directory", "") {}

		int exec(std::vector<std::string> args) {
			if(args.empty()) {
				char buffer[1024];
				getcwd(buffer, 1024);

				default_print(buffer);
				return 0;
			}

			std::string path;
			bool print_tree = false;

			std::vector<std::string> validArgs = {
				"-t", "-d"
			};

			for(auto& arg : args) {
				if(!io::vecContains(validArgs, arg) && arg.starts_with('-')) {
					info::error(std::string("Bad argument: ") + arg);
					return -1;
				}

				if(arg == "-t") {
					print_tree = true;
				}

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

			if(print_tree) {
				tree_print(dirpath);
				return 0;
			} else {
				default_print(dirpath);
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
