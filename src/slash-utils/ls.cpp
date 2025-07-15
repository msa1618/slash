#include "../command.h"
#include "../abstractions/iofuncs.h"

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <algorithm>
#include <math.h>

class Ls : public Command {
	public:
		Ls() : Command("ls", "Lists all files, directories, and links in a directory", "") {}

		int exec(std::vector<std::string> args) {
			char buffer[512];
			const char* path;

			if (args.empty() || args[0].empty()) {
				path = getcwd(buffer, 512);
			} else {
				path = args[0].c_str();
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
