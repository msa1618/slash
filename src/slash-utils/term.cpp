#include <unistd.h>
#include <signal.h>
#include "../command.h"
#include "../abstractions/iofuncs.h"

class Term : public Command {
	public:
	Term() : Command("term", "exit a process", "") {}

	int exec(std::vector<std::string> args) override {
		if(args.empty()) {
			io::print("term: send a signal to a process to exit\n"
								"usage: term <pid>\n");
		}

		pid_t pid;

		try {
			pid = std::stoi(args[0]);
		} catch (const std::exception& e) {
			io::print("Invalid PID: ");
			io::print(args[0]);
			io::print("\n");
			return -1;
		}

		kill(pid, SIGTERM);
		return 0;
	}
};

int main(int argc, char* argv[]) {
	Term term;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	term.exec(args);
	return 0;
}
