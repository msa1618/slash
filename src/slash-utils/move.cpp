#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"


#include <filesystem>
#include <unistd.h>
#include <sys/stat.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include "../help_helper.h"

class Move {
  private:
    int clear_history() {
      const char* home_c = getenv("HOME");
      std::string history_path = std::string(home_c) + "/.slash/util-files/move_logs.json";

      if(io::overwrite_file(history_path, "[\n]") != 0) {
        info::error("Failed to clear history.");
        return -1;
      }
    }

    int append_move_log(const std::string& src_file, const std::string& dest_dir) {
        const char* home_c = getenv("HOME");
        if (!home_c) {
            info::error("HOME environment variable not set");
            return -1;
        }
        std::string history_path = std::string(home_c) + "/.slash/util-files/move_logs.json";

        nlohmann::json j;

        // Read existing log file
        auto content = io::read_file(history_path);
        if (std::holds_alternative<std::string>(content)) {
            try {
                j = nlohmann::json::parse(std::get<std::string>(content));
            } catch (...) {
                // If parsing fails, start fresh
                j = nlohmann::json::array();
            }
        } else {
            j = nlohmann::json::array();
        }

        // Append new move log
        j.push_back({ {"from", src_file}, {"to", dest_dir} });

        // Write back
        std::string out_text = j.dump(4);
        if (io::overwrite_file(history_path, out_text) != 0) {
            info::error("Failed to update move_logs.json");
            return -1;
        }

        return 0;
    }

    int move(std::string src_file, std::string dest_dir) {
      if(dest_dir.ends_with("/")) dest_dir.pop_back();
      auto content = io::read_file(src_file);
      if(!std::holds_alternative<std::string>(content)) {
        std::string error = std::string("Failed to read file: ") + strerror(errno);
        info::error(error, errno);
        return -1;
      }

      std::string name = std::filesystem::path(src_file).filename();
      std::string cont = std::get<std::string>(content);
      std::string new_file_path = dest_dir + "/" + name;

      int code = io::create_file(new_file_path);
      if(code != 0) {
        std::string error = std::string("Failed to move " + new_file_path + ": ") + strerror(errno);
        info::error(error, errno);
        return errno;
      }

      if(io::overwrite_file(new_file_path, cont) != 0) {
        std::string error = std::string("Failed to write to moved file: ") + strerror(errno);
        info::error(error, errno);
        if(unlink(new_file_path.c_str()) != 0) {
          std::string error = std::string("Failed to delete new file: ") + strerror(errno);
          info::error(error, errno);
        return -1;
      }
    }

      struct stat st;
      if(stat(src_file.c_str(), &st) != 0) {
        info::error("Failed to stat original file. The file has been moved, but not the permissions.\n");
        if(unlink(src_file.c_str()) != 0) {
          std::string error = std::string("Failed to delete original file: ") + strerror(errno);
          info::error(error, errno);
          return -1;
        }
        return -1;
      }

      chmod(new_file_path.c_str(), st.st_mode);
      struct timespec times[2] = { st.st_atim, st.st_mtim };
      utimensat(AT_FDCWD, new_file_path.c_str(), times, 0);
      chown(new_file_path.c_str(), st.st_uid, st.st_gid);

      if(unlink(src_file.c_str()) != 0) {
        std::string error = std::string("Failed to delete original file: ") + strerror(errno);
        info::error(error, errno);
        return -1;
      }

      std::string full_src_path = std::filesystem::absolute(src_file).string();
      std::string full_dest_path = std::filesystem::absolute(new_file_path).string();
      return append_move_log(full_src_path, full_dest_path);
    }

    int undo(int n) {
      nlohmann::json j;

      const char* home_c = getenv("HOME");
      if (!home_c) {
          info::error("HOME environment variable not set");
          return -1;
      }
      std::string home(home_c);

      auto content = io::read_file(home + "/.slash/util-files/move_logs.json");
      if (!std::holds_alternative<std::string>(content)) {
          info::error("Failed to read move_logs.json");
          return -1;
      }

      try {
          j = nlohmann::json::parse(std::get<std::string>(content));
      } catch (const nlohmann::json::parse_error& e) {
          info::error(std::string("Error parsing move_logs.json: ") + e.what());
          return -1;
      }

      if (!j.is_array()) {
          info::error("move_logs.json is not an array");
          return -1;
      }

      if (n > j.size() || n <= 0) {
          info::error("Invalid undo number");
          return -1;
      }

      auto opr = j[j.size() - n];
      if (!opr.contains("from") || !opr.contains("to")) {
        info::error("Invalid log format");
        return -1;
      }

      std::string from = opr["to"];
      std::string to = opr["from"];

      int res = move(from, to);
      if (res != 0) {
          info::error("Undo move failed");
          return res;
      }

      return 0;
  }

  public:
    Move() {}

    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        io::print(get_helpmsg({
          "Moves files from one place to another, with cross-filesystem support",
          {
            "move <file> <new-dir>",
            "move [option]"
          },
          {
            {"-u", "--undo [n]", "Undo the last [n] change. If (n) is not specified, undo last change"},
            {"", "--clear-history", "Clear move logs"}
          },
          {
            {"move main.cpp ../", "Move main.cpp to the parent directory"},
            {"move -u", "Undo the last move"}
          },
          "",
          ""
        }));
        return 0;
      }

      std::vector<std::string> valid_args = {
        "-u",
        "--undo", "--clear-history"
      };

      for(auto& a : args) {
        if(!io::vecContains(valid_args, a) && a.starts_with('-')) {
          info::error("Invalid argument: " + a);
          return -1;
        }

        if(a == "--clear-history") {
          return clear_history();
        }

        if(a == "-u" || a == "--undo") {
          int n = 1;
          if(args.size() > 1) {
            try {
              n = std::stoi(args[1]);
            } catch (const std::invalid_argument&) {
              info::error("Invalid number for undo: " + args[1]);
              return -1;
            }
          }
          return undo(n);
        }

        if(args.size() < 2) {
          info::error("Not enough arguments provided.");
          return -1;
        }

        std::string src_file = args[0];
        std::string dest_dir = args[1];
        
        return move(src_file, dest_dir);
      }
    }
};

int main(int argc, char* argv[]) {
    Move move_command;
    std::vector<std::string> args(argv + 1, argv + argc);
    return move_command.exec(args);
}