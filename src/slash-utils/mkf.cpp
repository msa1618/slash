#include <unistd.h>
#include <fcntl.h>


#include "../abstractions/iofuncs.h"

#include <vector>
#include <string>

class Mkf : public Command {
    public:
        Mkf() : Command("mkf", "Creates a new file", "") {}

        int exec(std::vector<std::string> args) {
            if(args.empty()) {
                io::print("mkf: create a new file\n");
                return 0;
            }
            std::string name = args[0];

            int fd = open(name.c_str(), O_CREAT | O_WRONLY, 0644);
            if(fd == -1) {
                perror("Failed to create file");
                return -1;
            }

            io::print("Successfully created ");
            io::print(name);

            return 0;
        }
};

int main(int argc, char* argv[]) {
  Mkf mkf;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  mkf.exec(args);
  return 0;
}