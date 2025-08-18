#include "cd.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <algorithm>

#include "../abstractions/definitions.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include "../help_helper.h"

int cd(std::vector<std::string> args) {
  if(args.empty()) {
    io::print(get_helpmsg({
      "Change current directory",
      {"cd <dir>"},
      {},
      {
        {"cd ..", "Change to parent directory"},
        {"cd /bin", "Change to /bin"}
      },
      "",
      ""
    }));
    return 0;
  }

  bool create_alias_mode = false;

  if(args[0].starts_with("~")) {
    std::string home = getenv("HOME");
    io::replace_all(args[0], "~", home);
  }
  if(chdir(args[0].c_str()) != 0) {
    info::error("Failed to change directory to \"" + args[0] + "\": " + std::string(strerror(errno)), errno);
    return -1;
  }

  return 0;
}
