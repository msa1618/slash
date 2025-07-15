#ifndef AMBER_COMMAND_H
#define AMBER_COMMAND_H

#include <vector>
#include <unistd.h>
#include <functional>
#include <string>
#include "abstractions/iofuncs.h"

class Command {
	public:
		const char* name;
		const char* tldr;
		const char* man;

		Command(const char* name, const char* tldr, const char* man) : name(name), tldr(tldr), man(man) {};

		virtual int exec(std::vector<std::string> args);
};

template <typename T>
std::function<int(std::vector<const char*>)> wrapCommand(T& command) {
	return [&command](std::vector<const char*> argv) -> int {
		std::vector<std::string> args;
		for (const char* arg : argv)
			args.emplace_back(arg);
		return command.exec(args);
	};
}



#endif //AMBER_COMMAND_H
