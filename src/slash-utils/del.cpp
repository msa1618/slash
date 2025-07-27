#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <ctime>
#include <string>
#include <vector>
#include <iomanip>

#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../command.h"
#include <filesystem>

class Del : public Command {
	private:
		void create_dir_if_nonexistent(std::string path) {
			struct stat st;
			if(stat(path.c_str(), &st) != 0) {
				mkdir(path.c_str(), 0700);
			}
		}

		int trash_file(std::string path) {
	if (access(path.c_str(), F_OK) != 0) {
		info::error("File doesn't exist!");
		return -1;
	}

	std::string home = getenv("HOME");
	std::string trash_path = home + "/.local/share/Trash";

	create_dir_if_nonexistent(trash_path);
	create_dir_if_nonexistent(trash_path + "/files");
	create_dir_if_nonexistent(trash_path + "/info");

	time_t now = time(nullptr);
	char buf[128];
	strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", localtime(&now));

	std::filesystem::path p(path);
	std::string filename = p.filename().string();

	std::stringstream fileinfo;
	fileinfo << "[Trash Info]\n";
	fileinfo << "Path=" << std::filesystem::absolute(path) << "\n";
	fileinfo << "DeletionDate=" << buf << "\n";

	std::string info_path = trash_path + "/info/" + filename + ".trashinfo";
	if (io::create_file(info_path) != 0) {
		info::error("Failed to create info file");
		return -1;
	}
	if (io::overwrite_file(info_path, fileinfo.str()) != 0) {
		info::error("Failed to write to info file");
		return -1;
	}

	std::string file_dest = trash_path + "/files/" + filename;
	if (rename(path.c_str(), file_dest.c_str()) != 0) {
		std::string error = std::string("Failed to trash file: ") + strerror(errno);
		info::error(error, errno);
		return -1;
	}

	return 0;
}

		void delete_dirs_recursively(std::string dir_path, bool trash) {
			DIR* dir = opendir(dir_path.c_str());
			if (!dir) {
				info::error(strerror(errno), errno, dir_path);
				return;
			}

			struct dirent* entry;
			while ((entry = readdir(dir)) != nullptr) {
				std::string name = entry->d_name;
				if (name == "." || name == "..") continue;

				std::string full_path = dir_path + "/" + name;
				struct stat path_stat;
				if (lstat(full_path.c_str(), &path_stat) == -1) {
					info::error(strerror(errno), errno, full_path);
					continue;
				}

				if (S_ISDIR(path_stat.st_mode)) {
					delete_dirs_recursively(full_path, trash);
				} else {
					if (trash) {
						trash_file(full_path);
					} else {
						unlink(full_path.c_str());
					}
				}
			}
			closedir(dir);
			rmdir(dir_path.c_str());
		}


	public:
	Del() : Command("del", "Deletes files, directories, and links", "") {}

	int exec(std::vector<std::string> args) {
		if(args.empty()) {
			io::print("del: deletes one or more files and directories\n"
								"usage: del <file-1> <file-2> ...\n"
								"flags:\n"
								"-t | --trash:     trash file\n"
								"-r | --recursive: delete directories recursively\n"
								"-p | --prompt:    prompt before every deletion\n"
								"-i | --immediate: delete immediately, no prompt\n\n");
			return 0;
		}

		std::vector<std::string> valid_args = {
			"-t", "-p", "-i", "-r",
			"--trash", "--prompt", "--immediate", "--recursive"
		};

		bool prompt = false;
		bool trash = false;
		bool recursive = false;

		std::vector<std::string> filenames;

		for(auto& arg : args) {
			if(!io::vecContains(valid_args, arg) && arg.starts_with("-")) {
				info::error("Invalid argument \"" + arg + "\"\n");
				return -1;
			}

			if(arg == "-p" || arg == "--prompt") prompt = true;
			else if(arg == "-i" || arg == "--immediate") prompt = false;
			else if(arg == "-t" || arg == "--trash") trash = true;
			else if(arg == "-r" || arg == "--recursive") recursive = true;

			if(!arg.starts_with("-")) filenames.push_back(arg);
		}
		for(auto& path : filenames) {
			struct stat st;
			if(stat(path.c_str(), &st) != 0) {
				info::error("Failed to stat file: " + path, errno);
				continue;
			}

			if(prompt) {
				if(S_ISDIR(st.st_mode) && recursive) {
					info::warning("Are you sure you want to delete directory \"" + path + "\"? This will delete all its contents! (y/n)");
				} else {
					info::warning("Are you sure you want to delete file \"" + path + "\"? (y/n)");
				}
				char input[2];
				ssize_t bytesRead = read(STDIN_FILENO, input, 1);
				if(bytesRead < 0) {
					std::string error = std::string("Failed to read input: ") + strerror(errno);
					info::error(error, errno);
					return -1;
				}
				input[2] = '\0';
				if(tolower(input[0]) == 'n') continue;

				if(trash) {
					if(trash_file(path) != 0) {
						continue;
					}
				} else {
					if(S_ISDIR(st.st_mode) && recursive) {
						std::cout << trash;
						delete_dirs_recursively(path, trash);
					} else {
						if(unlink(path.c_str()) != 0) {
							std::string error = std::string("Failed to delete file: ") + strerror(errno);
							info::error(error, errno);
						}
					}
				}
			}
			if(trash) {
					if(trash_file(path) != 0) {
						continue;
					}
				} else {
				if(S_ISDIR(st.st_mode) && recursive) {
					delete_dirs_recursively(path, trash);
				} else {
					if(unlink(path.c_str()) != 0) {
						std::string error = std::string("Failed to delete file: ") + strerror(errno);
						info::error(error, errno);
					}
				}
		}
		}
	};
};

int main(int argc, char* argv[]) {
	Del del;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	del.exec(args);
	return 0;
}
