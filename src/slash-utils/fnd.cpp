#include <sys/stat.h>

#include <vector>
#include <string>
#include <filesystem>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../command.h"

class Fnd : public Command {
	private:
		std::string get_owner_name(int owner_id) {
			struct passwd* pw = getpwuid(owner_id);
			if(pw) return pw->pw_name;
			return "UNKNOWN";
		}

		std::string get_group_name(int group_id) {
			struct group* g = getgrgid(group_id);
			if(g) return g->gr_name;
			return "UNKNOWN";
		}

	int find(std::string inode_id = "", std::string dir = "", std::string name = "", std::string extension = "", std::string type = "", std::string owner = "", std::string group = "", std::string target = "") {
		std::string cwd;
		if(dir.empty()) {
			char buffer[512];
			if(!getcwd(buffer, 511)) {
				std::string error = std::string("Failed to get current directory: ") + strerror(errno);
				info::error(error, errno);
				return -1;
			}
			buffer[511] = '\0';
			dir = buffer;
			cwd = buffer;
		}

		DIR* d = opendir(dir.c_str());
		if(d == nullptr) {
			std::string error = std::string("Failed to open directory \"" + dir + ": ") + strerror(errno);
			info::error(error, errno);
			return -1;
		}
		struct dirent* entry;
		while((entry = readdir(d)) != nullptr) {
			std::string nm = entry->d_name;
			if(nm == "." || nm == "..") continue;

			std::string fullpath = dir.ends_with("/") ? dir + nm : dir + "/" + nm;

			struct stat st;
			if(stat(fullpath.c_str(), &st) != 0) {
				std::string error = std::string("Stat failed for \"" + fullpath + "\": ") + strerror(errno);
				info::error(error, errno);
				continue;
			}

			unsigned long long f_id = st.st_ino;
			std::string f_ext = std::filesystem::path(fullpath).extension();
			std::string f_type = [entry]() {
				switch(entry->d_type) {
					case DT_REG: return "file";
					case DT_DIR: return "dir";
					case DT_FIFO: return "fifo";
					case DT_SOCK: return "sock";
					case DT_LNK: return "link";
					default: return "unknown";
				}
			}();
			std::string f_owner = get_owner_name(st.st_uid);
			std::string f_group = get_group_name(st.st_gid);
			std::string f_target = [f_type, fullpath]() -> std::string {
				if(f_type != "link") return "NONE";
				char buffer[512];
				ssize_t bytesRead = readlink(fullpath.c_str(), buffer, 511);
				if(bytesRead < 0) {
					std::string error = std::string("Failed to get symlink target path: ") + strerror(errno);
					info::error(error, errno);
					return "";
				}
				buffer[bytesRead] = '\0';
				return {buffer};
			}();

			// Always recurse into directories
			if (f_type == "dir") {
				find(inode_id, fullpath, name, extension, type, owner, group, target);
			}

			// Apply filters *after* recursion
			if(!inode_id.empty() && std::stoull(inode_id) != f_id) continue;
			if(!name.empty() && name != nm) continue;
			if(!extension.empty() && extension != f_ext) continue;
			if(!type.empty() && type != f_type) continue;
			if(!owner.empty() && owner != f_owner) continue;
			if(!group.empty() && group != f_group) continue;
			if((type == "link" || type.empty()) && !f_target.empty() && target != f_target) continue;

			if(!cwd.empty() && fullpath.starts_with(cwd + "/")) {
				fullpath = "." + fullpath.substr(cwd.length());
			}

			std::string color;
			if (f_type == "dir") color = bold + blue;
			else if (f_type == "link") color = bold + orange;
			else if (f_type == "fifo") color = bold + red;
			else if (f_type == "sock") color = bold + magenta;
			else if (f_type == "file") color = green;
			else color = gray;

			std::vector<std::string> segs = io::split(fullpath, "/");
			for (size_t i = 0; i < segs.size() - 1; i++) {
				segs[i] = blue + segs[i] + "/" + reset;
			}
			if (!segs.empty()) {
				segs.back() = color + segs.back() + reset;
			}

			std::string colored_path;
			for (auto& seg : segs) {
				colored_path += seg;
			}

			io::print(colored_path + "\n");
		}
		closedir(d);
		return 0;
	}


	public:
		Fnd() : Command("fnd", "", "") {}

		int exec(std::vector<std::string> args) {
			if(args.empty()) {
				io::print("fnd: find a file based on its name, extension, type, owner, group, inode id, etc.\n"
									"usage: fnd [flags]\n"
									"flags:\n"
									"-n | --name: specify the name\n"
									"-e | --ext: specify the extension\n"
									"-t | --type: specify the type\n"
									"-o | --owner: specify owner\n"
									"-g | --group: specify group\n"
									"-i | --inode-id: specify inode id\n"
									"--target: specify target (if symlink)\n");
				return 0;
			}

			std::vector<std::string> valid_args = {
				"-n", "--name",
				"-e", "--ext",
				"-t", "--type",
				"-o", "--owner",
				"-g", "--group",
				"-i", "--inode-id",
				"--target"
			};

			std::string name, dir, extension, type, owner, group, inode_id, target;

			for(int i = 0; i < args.size(); i++) {
				if(!io::vecContains(valid_args, args[i]) && args[i].starts_with("-")) {
					info::error("Invalid argument \"" + args[i] + "\"\n");
					return -1;
				}

				if(args[i] == "-n" || args[i] == "--name") {
					if(i + 1 >= args.size()) {
						info::error("Missing name.");
						return -1;
					}
					name = args[++i];
				} else if(args[i] == "-e" || args[i] == "--ext") {
					if(i + 1 >= args.size()) {
						info::error("Missing extension.");
						return -1;
					}
					extension = args[++i];
				} else if(args[i] == "-t" || args[i] == "--type") {
					if(i + 1 >= args.size()) {
						info::error("Missing type.");
						return -1;
					}
					type = args[++i];
				} else if(args[i] == "-o" || args[i] == "--owner") {
					if(i + 1 >= args.size()) {
						info::error("Missing owner.");
						return -1;
					}
					owner = args[++i];
				} else if(args[i] == "-g" || args[i] == "--group") {
					if(i + 1 >= args.size()) {
						info::error("Missing group.");
						return -1;
					}
					group = args[++i];
				} else if(args[i] == "-i" || args[i] == "--inode-id") {
					if(i + 1 >= args.size()) {
						info::error("Missing inode ID.");
						return -1;
					}
					inode_id = args[++i];
				} else if(args[i] == "--target") {
					if(i + 1 >= args.size()) {
						info::error("Missing target.");
						return -1;
					}
					target = args[++i];
				} else if(!args[i].starts_with("-")) {
					dir = args[i];
				}
			}

			return find(inode_id, dir, name, extension, type, owner, group, target);
		}
};

int main(int argc, char* argv[]) {
	Fnd fnd;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	fnd.exec(args);
	return 0;
}
