#include <unistd.h>
#include "../abstractions/iofuncs.h"
#include "../abstractions/definitions.h"
#include "../command.h"

#include <sys/sysinfo.h>
#include <dirent.h>
#include <fcntl.h>

#include <sstream>

class Procs : public Command {
	public:
	Procs() : Command("procs", "shows currently running processes and their process ids", "") {}

	bool isDaemon(int pid) {
		std::stringstream ss;
		ss << "/proc/" << pid << "/stat";
		int fd = open(ss.str().c_str(), O_RDONLY);
		if(fd == -1) { perror("\x1b[1m\x1b[31merror\x1b[0m"); }

		char buffer[512];
		ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
		if(bytesRead == -1) {
			perror("\x1b[1m\x1b[31merror\x1b[0m"); return -1;
		}

		buffer[bytesRead] = '\0';

		std::vector<std::string> infs = io::split(std::string(buffer), ' ');

		int ppid = std::stoi(infs[3]); // Fourth field of the stats file is the ppid
		int tty = std::stoi(infs[6]);

		close(fd);

		return (ppid == 1) && (tty == 0); // Daemons are controlled by systemd/init (PID 1) and they do not control a terminal (TTY 0)
	}

	int exec(std::vector<std::string> args) {
		DIR* d = opendir("/proc/");
		struct dirent* entry;

		std::vector<std::string> validArgs = {
			"-n", "--sort-by-name"
		};

		bool sortByName = false;

		for(auto& arg : args) {
			if(!io::vecContains(validArgs, arg)) {
				io::print_err("\x1b[1m\x1b[31merror:\x1b[0m bad arguments\n");
				return -1;
			}

			if(arg == "-n" || arg == "--sort-by-name") sortByName = true;
		}

		std::vector<std::string> process_dirs;
		std::vector<std::pair<std::string, std::string>> pname_pid_pair;

		while((entry = readdir(d)) != NULL) {
			const char* name = entry->d_name;

			bool is_pid = true;
			for (int i = 0; name[i] != '\0'; ++i) {
				if (!std::isdigit(name[i])) {
					is_pid = false;
					break;
				}
			}

			if(!is_pid) continue;

			std::string full_path = "/proc/" + std::string(name) + "/comm";
			int fd = open(full_path.c_str(), O_RDONLY);
			if(fd == -1) { perror("procs"); return -1; }

			char buffer[256];
			ssize_t bytesRead = read(fd, buffer, sizeof(buffer) + 1);
			if(bytesRead < 0) {
				perror("procs");
				return -1;
			}

			buffer[bytesRead] = '\0';

			std::string pname = buffer;
			pname_pid_pair.push_back({pname, std::string(name)});
		}

		if(sortByName) std::sort(pname_pid_pair.begi);

		int longest_name_l = 0;
		int longest_pid_l = 0;
		for(auto& [name, pid] : pname_pid_pair) {
			if(name.length() > longest_name_l) longest_name_l = name.length();
			if(pid.length() > longest_pid_l) longest_pid_l = name.length();
		}


		for(auto& [name, pid] : pname_pid_pair) {
			int pid_int = std::stoi(pid);

			pid.resize(longest_pid_l, ' ');
			name.resize(longest_name_l, ' ');

			io::print(pid);
			io::print(" | ");
			if(isDaemon(pid_int)) {
				io::print(orange);
				io::print(name);
				io::print(reset);
			} else {
				io::print(blue);
				io::print(name);
				io::print(reset);
			}

			io::print("\n");
		}

	}
};

int main(int argc, char* argv[]) {
	Procs procs;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	procs.exec(args);
	return 0;
}