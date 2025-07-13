#include <unistd.h>
#include <signal.h>
#include "../command.h"
#include "../abstractions/iofuncs.h"

class Kill : public Command {
    public:
    Kill() : Command("kill", "Kills a process forcefully.", "") {}

    int exec(std::vector<std::string> args) override {
        if(args.empty()) {
            io::print("kill: terminal; a process abruptly and immediately\n"
                      "usage: kill <pid>");
        }

        pid_t pid;

        try {
            pid = std::stoi(args[0]);
        } catch (const std::exception& e) {
            io::print("Invalid PID: ");
            io::print(args[0].c_str());
            io::print("\n");
            return -1;
        }

        kill(pid, 9);
        return 0;
    }
};

int main(int argc, char* argv[]) {
	Kill kill;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	kill.exec(args);
	return 0;
}
 