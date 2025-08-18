#include "../abstractions/iofuncs.h"
#include "../help_helper.h"

#include <unistd.h>

class Ren {
  public:
    Ren() {}

    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        io::print(get_helpmsg({
          "Renames files",
          {
            "ren <old-name> <new-name>"
          },
          {},
          {},
          "",
          ""
        }));
        return 0;
      }

      const char* first_path = args[0].c_str();
      const char* sec_path = args[1].c_str();

      if(rename(first_path, sec_path) != 0) {
        perror("Failed to rename file");
        return 1;
      }
    }
};

int main(int argc, char* argv[]) {
  Ren ren;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  ren.exec(args);
  return 0;
}
