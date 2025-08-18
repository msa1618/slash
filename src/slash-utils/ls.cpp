
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../git/git.h"

#include "../help_helper.h"

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <math.h>

#include <unordered_map>
#include <filesystem>
#include <sys/ioctl.h>
#include <regex>

int get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
    return 80; // fallback width
}

class Ls {
private:
    std::string unknown_file = "\uea7b";
    std::string dir          = "\uf4d3";
    std::unordered_map<std::string, std::string> icon_map = {
        {".cpp",  "\ue646"},   // C++ blue
        {".c",    "\ue61e"},   // C blue
        {".toml", "\ue6b2"},  // TOML orange-ish
        {".html", "\ue736"},  // HTML orange
        {".yaml", "\ue8eb"},   // YAML blue
        {".js",   "\ue781"},  // JavaScript yellow
        {".ts",   "\ue69d"},   // TypeScript blue
        {".css",  "\U000f031c"},   // CSS blue
        {".rs",   "\ue7a8"},  // Rust orange
        {".r",    "\ue881"},   // R blue
        {".f90",  "\U000f121a"},  // Fortran orange (approx)
        {".swift","\ue699"},  // Swift orange
        {".java", "\ue738"},  // Java red
        {".kt",   "\ue634"},  // Kotlin red
        {".ttf",  "\uf031"},  // Font gray
        {".rb",   "\ue791"},  // Ruby red/pink
        {".lua",  "\ue620"},   // Lua teal
        {".fish", "\uf489"},
        {".sh",   "\uf489"},
        {".bash", "\uf489"},
        {".bat",  "\uf489"},
        {".ps",   "\uf489"},
        {".zsh",  "\uf489"},
        {".py",   "\ue73c"},   // Python greenish-blue
        {".json", "\ue60b"},  // JSON yellow (like JS)
        {".xml",  "\U000f05c0"},  // XML orange
        {".mp4",  "\uf03d"},  // Video purple
        {".png",  "\uf03e"},  // Image green
        {".mp3",  "\ue638"},  // Audio red
        {".pdf",  "\U000f0219"}   // PDF bright red
    };

    std::string get_with_icon(std::string path) {
        std::filesystem::path p(path);
        std::string name = p.filename().string();
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            std::string error = std::string("Failed to stat \"" + name + "\": ") + strerror(errno);
            info::error(error, errno);
            return "";
        }
        std::string type = [name, st]() -> std::string {
            if (S_ISREG(st.st_mode))  return "file";
            if (S_ISDIR(st.st_mode))  return "dir";
            if (S_ISLNK(st.st_mode))  return "link";
            if (S_ISFIFO(st.st_mode)) return "fifo";
            if (S_ISSOCK(st.st_mode)) return "sock";
            return "unknown";
        }();

        std::string icon = unknown_file, color = "";
        std::string name_to_be_pushed;

        if (type == "file") {
            if (p.has_extension()) {
                auto it = icon_map.find(p.extension());
                if (it != icon_map.end()) {
                    icon = icon_map[p.extension()];
                }
            }
        }

        if (type == "dir") {
            color = "\x1b[38;5;3m";
            icon  = dir;
        }

        if (type == "link") {
            struct stat tmp;
            if (stat(path.c_str(), &tmp) != 0) { // Broken symlink
                icon = "\uf127";
                color = "\x1b[38;5;1m";
            } else {
                char buf[PATH_MAX];
                ssize_t bytesRead = readlink(path.c_str(), buf, PATH_MAX - 1);
                if (bytesRead < 0) {
                    icon = "\uf127";
                    color = "\x1b[38;5;1m";
                }
                icon = get_with_icon(buf);
                buf[bytesRead] = '\0';
            }
        }

        if (type == "fifo") {
            icon = "\U000f07e5";
            color = "\x1b[38;5;13m";
        }

        if (type == "sock") {
            icon = "\U000f0427"; // literally what does a socket look like
            color = "\x1b[38;5;50m";
        }

        return icon;
    }

    int default_print(std::string dir_path, bool print_hidden, bool with_icons, bool with_git) {
        DIR* d = opendir(dir_path.c_str());
        if(!d) {
            std::string error = std::string("Failed to open directory" + dir_path + ": ") + strerror(errno);
            info::error(error, errno);
        }

        std::vector<std::tuple<std::string, int, std::string>> entries;

        struct dirent* entry;
        GitRepo repo(dir_path);

        while((entry = readdir(d)) != NULL) {
            std::string name = entry->d_name;
            std::string fullpath = dir_path + "/" + name;
            std::string color;
            std::string git_char;

            if((name == "." || name == ".." || name.starts_with(".")) && !print_hidden) continue;

            if(with_git) {
                if(repo.get_repo() == nullptr) {
                    info::error("The directory is not a git repository.");
                    return -1;
                }

                std::string repo_root = repo.get_root_path();
                if (!repo_root.empty()) {
                    std::filesystem::path file_path = std::filesystem::absolute(fullpath);
                    std::filesystem::path root_path = std::filesystem::path(repo_root);
                    std::string relative_path = std::filesystem::relative(file_path, root_path).string();

                    std::string status = repo.get_file_status(relative_path);
                    git_char = status;

                    if (git_char == "U") {
                        color = blue;  // Red (unmodified)
                    } else if (git_char == "M") {
                        color = "\033[33m";  // Yellow (modified)
                    } else if (git_char == "S") {
                        color = "\033[32m";  // Green (staged)
                    } else if (git_char == "I") {
                        color = "\033[90m";  // Gray (ignored)
                    } else if (git_char == "R") {
                        color = "\033[36m";  // Cyan (renamed)
                    } else if (git_char == "N") {
                        color = gray;   // Reset (untracked)
                    } else {
                        color = "\033[0m";   // Fallback/reset
                    }
                } else {
                    info::error("Failed to get Git root path.");
                    return -1;
                }
            } else {
                switch(entry->d_type) {
                    case DT_REG: color = green; break;
                    case DT_DIR: color = blue; break;
                    case DT_LNK: color = orange; break;
                    case DT_FIFO: color = "\x1b[35m"; break;
                    default: color = gray; 
                }
    }

    std::stringstream fullname;
    if(with_icons) fullname << color << get_with_icon(fullpath) + " " << reset;
    fullname << color << name << reset;
    if(with_git) {
        if(!git_char.empty()) fullname << color << " (" << git_char << ") " << reset;
    } 

    entries.push_back({fullname.str(), fullname.str().length(), git_char});
}
        std::sort(entries.begin(), entries.end(), [](std::tuple<std::string, int, std::string>& a, std::tuple<std::string, int, std::string>& b) {
            auto first = io::strip_ansi(std::get<0>(a));
            auto sec = io::strip_ansi(std::get<0>(b));
            return io::strip_ansi(first) < io::strip_ansi(sec);
        });
 
        int current_width_available = get_terminal_width();
        int longest_name_length = 0;

        for(auto& [name, nlength, git_char] : entries) {
            int length_excluding_ansi = io::strip_ansi(name).length();
            if(length_excluding_ansi > longest_name_length) longest_name_length = length_excluding_ansi + 1; // +1 for padding
        }

        for(auto& [name, nlength, git_char] : entries) {
            int stripped_len = io::strip_ansi(name).length();
            if (stripped_len < longest_name_length)
                name.append(longest_name_length - stripped_len, ' ');

            if(io::strip_ansi(name).length() > current_width_available) {
                io::print("\n");
                current_width_available = get_terminal_width();
            }
            io::print(name);
            current_width_available -= io::strip_ansi(name).length();
        }    

        io::print("\n");
    }

    int tree_print(std::string dir_path, bool print_hidden, std::vector<bool> last_entry_stack = {}) {
        DIR *dir = opendir(dir_path.c_str());
        if (!dir) {
            info::error(strerror(errno), errno, dir_path);
            return -1;
        }

        struct dirent *entry;
        std::vector<std::string> names;

        // Collect entries
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if ((name == "." || name == "..") || (!print_hidden && name.starts_with("."))) {
                continue;
            }
            names.push_back(name);
        }

        std::sort(names.begin(), names.end());
        for (size_t i = 0; i < names.size(); ++i) {
            bool is_last = (i == names.size() - 1);
            std::string name = names[i];
            std::string path = dir_path + "/" + name;

            struct stat path_stat;
            if (lstat(path.c_str(), &path_stat) == -1) {
                perror("lstat");
                continue;
            }

            // Print indentation and vertical lines for ancestor levels
            for (size_t lvl = 0; lvl < last_entry_stack.size(); ++lvl) {
                if (last_entry_stack[lvl]) {
                    io::print("  ");    // no vertical line, last entry at this level
                } else {
                    io::print("│ ");
                }
            }

            // Print branch for current entry
            io::print(is_last ? "└─" : "├─");

            if (S_ISDIR(path_stat.st_mode)) {
                io::print(bold + blue + name + reset + "\n");
                auto new_stack = last_entry_stack;
                new_stack.push_back(is_last);
                tree_print(path, print_hidden, new_stack);
            } else if (S_ISLNK(path_stat.st_mode)) {
                char buf[1024];
                ssize_t len = readlink(path.c_str(), buf, sizeof(buf) - 1);
                if (len != -1) {
                    buf[len] = '\0';
                    io::print(bold + orange + name + reset + " -> " + buf + "\n");
                } else {
                    io::print(orange + name + reset + " -> [unreadable]\n");
                }
            } else if (S_ISSOCK(path_stat.st_mode)) {
                io::print(bold + magenta + name + reset + "\n");
            } else if (S_ISFIFO(path_stat.st_mode)) {
                io::print(bold + red + name + reset + "\n");
            } else {
                io::print(green + name + reset + "\n");
            }
        }

        closedir(dir);
        return 0;
    }

public:
    Ls() {}

    int exec(std::vector<std::string> args) {
        if (args.empty()) {
            char buffer[1024];
            getcwd(buffer, 1024);

            default_print(buffer, false, false, false);
            return 0;
        }

        std::string path;
        bool print_tree = false;
        bool print_hidden = false;
        bool with_icons = false;
        bool with_git = false;

        std::vector<std::string> validArgs = {
            "-t", "-a", "-i", "-g", "-h"
            "--tree", "--all", "--icons", "--git", "--help"
        };

        for (auto& arg : args) {
            if (!io::vecContains(validArgs, arg) && arg.starts_with('-')) {
                info::error(std::string("Bad argument: ") + arg);
                return -1;
            }

            if (arg == "-t" || arg == "--tree") print_tree = true;
            if (arg == "-a" || arg == "--all") print_hidden = true;
            if (arg == "-i" || arg == "--icons") with_icons = true;
            if (arg == "-g" || arg == "--git") with_git = true;

            if (arg == "-h" || arg == "--help") {
                io::print(get_helpmsg({
                    "List all files and directories in a specified directory",
                    {
                        "ls",
                        "ls <directory>",
                        "ls [option] <directory>"
                    },
                    {
                        {"-t", "--tree", "Prints content recursively"},
                        {"-a", "--all", "Print hidden files too"},
                        {"-i", "--icons", "Print with nerd font icons"},
                        {"-g", "--git", "Print with git statuses"}
                    },
                    {
                        {"ls", "Print all files in the current directory"},
                        {"ls /dev", "Print all files in /dev"},
                        {"ls -t -a /", "Print every single file in the system (Go on try it)"},
                    },
                    "",
                    ""
                }));
            }

            if (!arg.starts_with("-")) {
                path = arg;
            }
        }

        std::string dirpath = path;
        if (path.empty()) {
            char buffer[1024];
            getcwd(buffer, 1024);
            dirpath = buffer;
        }

        if (print_tree) {
            tree_print(dirpath, print_hidden);
            return 0;
        } else {
            default_print(dirpath, print_hidden, with_icons, with_git);
            return 0;
        }
    }
};

int main(int argc, char* argv[]) {
    Ls ls;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    ls.exec(args);
    return 0;
}
