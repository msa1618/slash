#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"

#include "../tui/tui.h"
#include <csignal>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <boost/regex.hpp>
#include <filesystem>

int height = Tui::get_terminal_height() - 2;
int files_height = Tui::get_terminal_height() - 4;

std::function<void()> renderer = nullptr;

void winch_handler(int sig) { 
  height = Tui::get_terminal_height() - 2;
  int files_height = Tui::get_terminal_height() - 4;

  if(renderer != nullptr) renderer();
}
 
class Srch {
  private:
    int files_srch(bool no_icons, std::string dirpath) {
      Tui::clear();

      std::string buffer;
      std::vector<std::pair<std::string, std::string>> files;

      auto redraw = [&](std::string regex, int selected_index, int starting_index){
        std::vector<std::pair<std::string, std::string>> filtered;
        Tui::clear();

        io::print(yellow + "Current directory: " + reset + dirpath + "\n");
        Tui::print_separator();

        DIR* d = opendir(dirpath.c_str());
        if(!d) return; // TODO: Next version, make a popup component in the tui library

        struct dirent* entry;
        while((entry = readdir(d)) != nullptr) {
          std::string name = entry->d_name;
          if(name == ".") continue;

          boost::regex reg;
          try {
            if(!buffer.empty()) reg = boost::regex(regex, boost::regex_constants::icase);
            else reg = boost::regex(".*");
          } catch(...) {
            reg = boost::regex(".*");
          }

          if(!boost::regex_search(name, reg)) continue;

          std::string type = [&]() -> std::string {
            struct stat st;
            std::string fullpath = dirpath + "/" + name;

            if(lstat(fullpath.c_str(), &st) != 0) {
              return "file";
            };

            if(S_ISREG(st.st_mode))  return "file";
            if(S_ISDIR(st.st_mode))  return "dir";
            if(S_ISLNK(st.st_mode))  return "link";
            if(S_ISFIFO(st.st_mode)) return "fifo";
            if(S_ISSOCK(st.st_mode)) return "sock";
            if(S_ISBLK(st.st_mode))  return "blk";
            if(S_ISCHR(st.st_mode))  return "char";

            // fallback
            return "file";
          }();

          filtered.push_back({type, name});
        }

        std::sort(filtered.begin(), filtered.end(), [](const auto &a, const auto &b) {
            bool a_dir = a.first == "dir";
            bool b_dir = b.first == "dir";

            if (a_dir != b_dir) return a_dir;        // dirs first
            return a.second < b.second;             // then sort alphabetically by name
        });

        if(!files.empty()) {
          auto it = std::find_if(filtered.begin(), filtered.end(), [&](std::pair<std::string, std::string>& pair){
            return pair.second == files[selected_index].second;
          });
          if(it != filtered.end()) {
            selected_index = std::distance(filtered.begin(), it);
          } else selected_index = 0;
        }

        files = filtered; 

        for(int i = starting_index; i < std::min(starting_index + files_height, (int)files.size()); i++) {
          auto type = files[i].first;
          auto name = files[i].second;

          std::string ic; // icon and color

          if(type == "dir") ic += yellow;
          if(type == "link") ic += orange;
          if(type == "fifo") ic += magenta;
          if(type == "sock") ic += blue;
          if(type == "blk") ic += cyan;
          if(type == "char") ic += gray;

          if(!no_icons) {
            if(type == "file") ic += "\uf4a5";
            if(type == "dir") ic += "\uf4d3";
            if(type == "link") ic += "\uf0c1";
            if(type == "fifo") ic += "\U000f07e5";
            if(type == "sock") ic += "\U000f0427";
            if(type == "blk") ic += "\uf0a0";
            if(type == "char") ic += "\uf11c";
          }

          std::stringstream ss;
          if(i == selected_index) ss << bg_gray;
          ss << ic << " " << name << reset << "\n";
          io::print(ss.str());
        }

        int i = 0;
        while(files.size() + i < files_height) {
          io::print("\n");
          i++;
        }

        Tui::print_separator();
        io::print(orange + "> " + reset + buffer);
      };
      auto redraw_line = [&](int selected_index, int starting_index, bool highlight) {
        Tui::turn_cursor(OFF);
        Tui::move_cursor_to((selected_index - starting_index) + 3, 0);
        if(highlight) io::print(bg_gray);

        std::string ic; // icon and color
        std::string type = files[selected_index].first;

        if(type == "dir") ic += yellow;
        if(type == "link") ic += orange;
        if(type == "fifo") ic += magenta;
        if(type == "sock") ic += blue;
        if(type == "blk") ic += cyan;
        if(type == "char") ic += gray;

        if(!no_icons) {
          if(type == "file") ic += "\uf4a5";
          if(type == "dir") ic += "\uf4d3";
          if(type == "link") ic += "\uf0c1";
          if(type == "fifo") ic += "\U000f07e5";
          if(type == "sock") ic += "\U000f0427";
          if(type == "blk") ic += "\uf0a0";
          if(type == "char") ic += "\uf11c";
        }

        io::print(ic + " " + files[selected_index + starting_index].second + reset);
        io::print("\x1b[0K");
        Tui::move_cursor_to(Tui::get_terminal_height(), 2 + buffer.size());
      };

      int selected_index = 0,
          height = Tui::get_terminal_height() - 4,
          starting_index = 0;
  
      renderer = [&](){
        redraw(buffer, selected_index, starting_index); // for files
      };       

      redraw(".*", selected_index, starting_index);

      while(true) {
        std::string keypress = Tui::get_keypress();
        if(keypress.empty()) continue;

        if(keypress == "Backspace") {
          if(!buffer.empty()) {
            buffer.pop_back();
            io::print("\b \b");
          }
        }

        if(keypress == "ArrowDown") {
            if(selected_index + 1 < files.size()) {
                selected_index++;
                if(selected_index >= starting_index + files_height) {
                  starting_index = selected_index - files_height + 1;
                  redraw(buffer, selected_index, starting_index);
                } else if(starting_index != 0) {
                    redraw(buffer, selected_index, starting_index);
                }
                else {
                  redraw_line(selected_index - 1, starting_index, false);
                  redraw_line(selected_index, starting_index, true);
                }
            }
            continue;
        }

        if(keypress == "ArrowUp") {
            if(selected_index > 0) {
                selected_index--;
                if(selected_index < starting_index) {
                    starting_index = selected_index;
                    redraw(buffer, selected_index, starting_index);
                } else if(starting_index != 0) {
                    redraw(buffer, selected_index, starting_index);
                } else {
                  redraw_line(selected_index + 1, starting_index, false);
                  redraw_line(selected_index, starting_index, true);
                }
            }
            continue;
        }

        if(keypress == "Enter") {
          if(files[selected_index].first == "dir") {
            std::filesystem::path canonical = std::filesystem::canonical(std::filesystem::path(dirpath + "/" + files[selected_index].second));
            return files_srch(no_icons, canonical.string());
          } else {
            if(!files.empty() && selected_index + starting_index < files.size()) {
              disable_raw_mode();
              Tui::switch_to_normal();
              io::print(files[selected_index].second + "\n"); // For piping
              Tui::turn_cursor(ON);
              _exit(0);
            }
          }

        }

        if(keypress.size() == 1 && isprint(keypress[0])) {
          io::print(keypress);
          buffer.push_back(keypress[0]);
        }

        Tui::clear();
        redraw(buffer, selected_index, starting_index);
      }

      Tui::move_cursor_to(Tui::get_terminal_height(), 3);
      return 0;
    }

    int history_srch(bool no_icons) {
      static std::unordered_map<std::string, std::string> icons = {
        {"vim", "\ue62b"},
        {"neovim", "\ue6ae"},
        {"nano", "\ue838"},
        {"help", "\U000f02d7"},
        {"git", "\ue702"},
        {"subversion", "\ue8b5"},
        {"fzf", "\uec0d"},
        {"srch", "\uec0d"},
        {"emacs", "\ue632"},
        {"bash", "\ue760"},
        {"zsh", "\uf489"},
        {"fish", "\uf489"},
        {"powershell", "\ue86c"},
        {"ftp", "\ueaec"},
        {"curl", "\U000f059f"},
        {"htop", "\U000f07cc"},
        {"btop", "\U000f07cc"},
        {"tar", "\ueaef"},
        {"bzip2", "\ueaef"},
        {"xz", "\ueaef"},
        {"gzip", "\ueaef"},
        {"7z", "\ueaef"},
        {"rar", "\ueaef"},
        {"unrar", "\ueaef"},
        {"unzip", "\ueaef"},
        {"man", "\U000f00ba"},
        {"python", "\ue73c"},
        {"g++", "\ue61d"},
        {"clang", "\ue61d"},
        {"apt", "\U000f03d6"},
        {"pacman", "\U000f03d6"},
        {"npm", "\U000f03d6"},
        {"pip", "\U000f03d6"},
        {"cargo", "\U000f03d6"},
        {"go", "\ue627"},
        {"tmux", "\uebc8"},
        {"ssh", "\ueb3a"},
        {"ed", "\uf044"},
        {"datetime", "\U000f00ed"},
        {"java", "\U000f06ca"},
        {"redis-cli", "\ue76d"},
        {"mysql", "\uf1c0"},
        {"sqlite3", "\uf1c0"},
        {"psql", "\uf1c0"},
        {"kubectl", "\ue81d"},
        {"docker", "\ue7b0"},
        {"acart", "\uee26"},
        {"ansi", "\ue22b"},
        {"cmsh", "\U000f0a9a"},
        {"clear", "\ueabf"},
        {"csv", "\uf525"},
        {"dump", "\U000f12a7"},
        {"xxd", "\uef42"},
        {"echo", "\U000f0284"},
        {"printf", "\U000f0284"},
        {"pager", "\U000f0bb0"},
        {"less", "\U000f0bb0"},
        {"more", "\U000f0bb0"},
        {"cat", "\uea7b"},
        {"bat", "\uea7b"},
        {"rf", "\uea7b"},
        {"sumcheck", "\U000f0483"},
        {"md5sum", "\U000f0483"},
        {"sha256sum", "\U000f0483"},
        {"sleep", "\U000f0904"}
    };

      Tui::clear();
      std::string home = getenv("HOME");
      auto content = io::read_file(home + "/.slash/.slash_history");
      if(!std::holds_alternative<std::string>(content)) {
          disable_raw_mode();
          Tui::switch_to_normal();
          info::error("Failed to open " + home + "/.slash/.slash_history: " + std::string(strerror(errno)));
          Tui::turn_cursor(ON);
          _exit(errno);
      }

      std::vector<std::string> history = io::split(std::get<std::string>(content), "\n");
      std::vector<std::string> filtered;

      int selected_index = 0;
      int starting_index = 0;
      std::string buffer;

      auto redraw = [&](int &selected_index, int starting_index) {
          // Rebuild filtered list
          filtered.clear();
          boost::regex reg;
          try {
              reg = buffer.empty() ? boost::regex(".*") : boost::regex(buffer, boost::regex_constants::icase);
          } catch(...) {
              reg = boost::regex(".*");
          }
          for(auto &elm : history) {
              if(boost::regex_search(elm, reg)) filtered.push_back(elm);
          }

          // Clamp selected_index
          if(selected_index >= (int)filtered.size()) selected_index = filtered.empty() ? 0 : filtered.size() - 1;

          Tui::clear();
          for(int i = starting_index; i < std::min(starting_index + height, (int)filtered.size()); i++) {
              if(i == selected_index) io::print(bg_gray);

              if(!no_icons) {
                std::string command = io::split(filtered[i], " ")[0];
                if(command.starts_with("sudo")) command = io::split(filtered[i], " ")[1]; // Get icon for command instead of sudo

                auto it = icons.find(command);
                if(it != icons.end()) {
                  io::print(icons[io::split(command, " ")[0]]);
                } else io::print("\uf489");

                io::print(" ");
              }

              io::print(filtered[i] + reset + "\n");
          }

          int extra = 0;
          while(filtered.size() + extra < Tui::get_terminal_height()) {
              io::print("\n");
              extra++;
          }

          Tui::print_separator();
          io::print(orange + "> " + reset + buffer);
      };

      auto redraw_line = [&](int selected_index, int starting_index, bool highlight) {
          int row = (selected_index - starting_index) + 1;
          Tui::move_cursor_to(row, 0);
          if(highlight) io::print(bg_gray);

          if(!no_icons) {
              std::string command = io::split(filtered[starting_index + selected_index], " ")[0];
              if(command.starts_with("sudo")) command = io::split(filtered[starting_index + selected_index], " ")[1]; // Get icon for command instead of sudo

              auto it = icons.find(command);
              if(it != icons.end()) {
               io::print(icons[io::split(command, " ")[0]]);
             } else io::print("\uf489");

            io::print(" ");
          }

          io::print("\x1b[0K"); // Clear line
          io::print(filtered[selected_index] + reset);
          Tui::move_cursor_to(Tui::get_terminal_height(), buffer.size() + 2);
      };

      redraw(selected_index, starting_index);
      renderer = [&](){
        redraw(selected_index, starting_index);
      };

      while(true) {
          std::string keypress = Tui::get_keypress();
          if(keypress.empty()) continue;

          if(keypress == "Backspace") {
              if(!buffer.empty()) {
                  buffer.pop_back();
                  io::print("\b \b");
              }
          }

          if(keypress == "ArrowDown") {
              if(selected_index + 1 < (int)filtered.size()) {
                  selected_index++;
                  if(selected_index >= starting_index + height) {
                      starting_index = selected_index - height + 1;
                      redraw(selected_index, starting_index);
                  } else if(starting_index != 0) {
                      redraw(selected_index, starting_index);
                  } else {
                      redraw_line(selected_index - 1, starting_index, false);
                      redraw_line(selected_index, starting_index, true);
                  }
              }
              continue;
          }

          if(keypress == "ArrowUp") {
              if(selected_index > 0) {
                  selected_index--;
                  if(selected_index < starting_index) {
                      starting_index = selected_index;
                      redraw(selected_index, starting_index);
                  } else if(starting_index != 0) {
                      redraw(selected_index, starting_index);
                  } else {
                      redraw_line(selected_index + 1, starting_index, false);
                      redraw_line(selected_index, starting_index, true);
                  }
              }
              continue;
          }

          if(keypress == "Enter") {
              disable_raw_mode();
              Tui::switch_to_normal();
              if(!filtered.empty()) io::print(filtered[selected_index] + "\n");
              Tui::turn_cursor(ON);
              _exit(0);
          }

          if(keypress.size() == 1 && isprint(keypress[0])) {
              buffer.push_back(keypress[0]);
              selected_index = 0;
              starting_index = 0;
          }

          redraw(selected_index, starting_index);
      }
  }

    int srch(bool no_icons) {
      enable_raw_mode();
      Tui::switch_to_alternate();
      Tui::clear();
      Tui::turn_cursor(OFF);

      signal(SIGINT, Tui::cleanup_and_exit);
      signal(SIGTERM, Tui::cleanup_and_exit);
      signal(SIGWINCH, winch_handler);

      auto render = [&](){
        Tui::clear();
        std::stringstream ss;
        ss << "Welcome to " << green << "srch!\n\n" << reset;
        ss << blue << "Ctrl+F" << reset << " to search files  \n";
        ss << blue << "Ctrl+P" << reset << " to search history\n";
        ss << red  << "     q" << reset << " to exit\n";

        Tui::print_in_center_of_terminal(ss.str());

        while(true) {
          std::string keypress = Tui::get_keypress();
          if(keypress.empty()) continue;

          if(keypress == "q" || keypress == "Q") {
            disable_raw_mode();
            Tui::cleanup_and_exit(0);
          }

          if(keypress == "Ctrl+F") {
            char buffer[PATH_MAX];
            getcwd(buffer, PATH_MAX);

            return files_srch(no_icons, buffer);
          }
          if(keypress == "Ctrl+P") return history_srch(no_icons);

          continue;
        }
      };

      renderer = [render](){ render(); };
      return render();
    }

  public:
    Srch() {}

    int exec(std::vector<std::string> args) {
      return srch(false);
    }
};

int main(int argc, char* argv[]) {
  Srch srch;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  srch.exec(args);
  return 0;
}