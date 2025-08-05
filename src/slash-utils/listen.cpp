#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/limits.h>

#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../command.h"

class Listen : public Command {
  private:
    std::string get_process_name(std::string pid) {
      std::string comm_path = "/proc/" + pid + "/comm";
      auto content = io::read_file(comm_path);
      if(!std::holds_alternative<std::string>(content)) {
        info::error(std::string("Failed to read file to get process name: ") + strerror(errno), errno);
        return "";
      }

      return std::get<std::string>(content);
    }

    int listen(std::string filepath, bool creation, bool mod, bool del, bool rd, bool open, bool cl) {
      #define EVENT_SIZE  (sizeof(struct inotify_event))
      #define BUF_LEN     (1024 * (EVENT_SIZE + 16))

      int fd = inotify_init();
      if (fd < 0) {
          std::string error = std::string("Failed to initialize listening: ") + strerror(errno);
          info::error(error, errno);
          return 1;
      }

      uint32_t flags = 0;
      if (creation) flags |= IN_CREATE;
      if (mod)      flags |= IN_MODIFY;
      if (del)      flags |= IN_DELETE | IN_DELETE_SELF;
      if (rd)       flags |= IN_ATTRIB;
      if (open)     flags |= IN_OPEN;
      if (cl)       flags |= IN_CLOSE_WRITE | IN_CLOSE_NOWRITE;

      int wd = inotify_add_watch(fd, filepath.c_str(), flags);

      if (wd == -1) {
          auto error = std::string("Failed to start watching file: ") + strerror(errno);
          info::error(error, errno);
          close(fd);
          return 1;
      }

      char buffer[BUF_LEN];

      while (true) {
          int length = read(fd, buffer, BUF_LEN);
          if (length < 0) {
              auto error = std::string("Failed to watch file: ") + strerror(errno);
              info::error(error, errno);
              break;
          }

          for (int i = 0; i < length;) {
              struct inotify_event* event = (struct inotify_event*)&buffer[i];
              std::string name = event->len ? event->name : filepath;
              std::string msg;

              if (event->mask & IN_CREATE)        msg += "- Created file";
              if (event->mask & IN_MODIFY)        msg += cyan + "- Modified file" + reset;
              if (event->mask & IN_DELETE)        msg += red + "- Deleted file" + reset;
              if (event->mask & IN_DELETE_SELF)   msg += red + "- Watched file deleted" + reset;
              if (event->mask & IN_ATTRIB)        msg += yellow + "- File metadata changed" + reset;
              if (event->mask & IN_OPEN)          msg += green + "- Opened file" + reset;
              if (event->mask & IN_CLOSE_WRITE)   msg += blue + "- Closed file (write) " + reset;
              if (event->mask & IN_CLOSE_NOWRITE) msg += blue + "- Closed file (no write) " + reset;
              if (event->mask & IN_MOVED_FROM)    msg += magenta + "- Moved file from " + reset;
              if (event->mask & IN_MOVED_TO)      msg += magenta + "- Moved file to " = reset;

              if (!msg.empty())
                  io::print(msg + " → " + name + "\n");

              i += EVENT_SIZE + event->len;
          }
      }

      inotify_rm_watch(fd, wd);
      close(fd);
      return 0;
    }

  public:
    Listen() : Command("listen", "", "") {}

    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        std::string help = R"(listen: listen for file changes like modification, deletion... By default, it listens for all events.
usage: listen <filepath>
       listen [options] <filepath>

flags:
  -d | --deletion:     listen for deletion
  -m | --modification: listen for modification
  -o | --opened:       listen for opening
  -r | --read:         listen for reading
  -c | --close:        listen for closing
  -C | --creation:     listen for creation

tip: you can combine multiple short arguments into one argument to save time
(ex. listen -dmo will listen for deletion, modification, and opening)
)";
        io::replace_all(help, "tip:", green + bold + "tip:" + reset);
        io::print(help);
        return 0;
      }

      std::vector<std::string> valid_args = {
          "-d", "--deletion",
          "-m", "--modification",
          "-o", "--opened",
          "-r", "--read",
          "-c", "--close",
          "-C", "--creation"
      };

      std::string filepath;

      bool creation = false;
      bool deletion = false;
      bool modif    = false;
      bool opened   = false;
      bool read     = false;
      bool close    = false;

      for(auto& arg : args) {
        bool is_combined_arg = !arg.starts_with("--") && arg.starts_with("-") && arg.length() > 2;
        if(!io::vecContains(valid_args, arg) && arg.starts_with("-") && !is_combined_arg) {
          info::error("Invalid argument \"" + arg + "\"");
          return -1;
        }

        if (arg == "-d" || arg == "--deletion")          deletion = true;
        else if (arg == "-m" || arg == "--modification") modif = true;
        else if (arg == "-o" || arg == "--opened")       opened = true;
        else if (arg == "-r" || arg == "--read")         read = true;
        else if (arg == "-c" || arg == "--close")        close = true;
        else if (arg == "-C" || arg == "--creation")     creation = true;
        else if (!arg.starts_with("-"))                  filepath = arg;

        if (is_combined_arg) {
          for (int i = 1; i < arg.length(); i++) {
              char c = arg[i];
              switch (c) {
                  case 'd': deletion = true; break;
                  case 'm': modif    = true; break;
                  case 'o': opened   = true; break;
                  case 'r': read     = true; break;
                  case 'c': close    = true; break;
                  case 'C': creation = true; break;
                  default:
                      info::error(std::string("Invalid combined flag: -") + c);
                      return -1;
              }
          }
        }
      }

      return listen(filepath, creation, modif, deletion, read, opened, close);
    }
};

int main(int argc, char* argv[]) {
    Listen listen;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    listen.exec(args);
    return 0;
}