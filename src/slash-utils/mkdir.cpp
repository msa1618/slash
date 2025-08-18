#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "../abstractions/iofuncs.h"

#include <vector>
#include <string>

class Mkdir {
    public:
        Mkdir() {}

        int exec(std::vector<std::string> args) {
            if(args.empty()) {
                io::print(green + "Function" + "\n  Create a directory");
                return 0;
            }
            std::string name = args[0];

            int fd = mkdir(name.c_str(), 0755);
            if(fd != 0) {
                perror("Failed to create directory");
                return -1;
            }

            io::print("Successfully created ");
            io::print(name);

            return 0;
        }
};

int main(int argc, char* argv[]) {
  Mkdir mkdir;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  mkdir.exec(args);
  return 0;
}