#include <unistd.h>
#include <fcntl.h>

#include "../command.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <vector>
#include <string>
#include <filesystem>
#include <sys/stat.h>

class Create : public Command {
private:
    bool contains_invalid_chars(std::string name) {
        return name.starts_with("/") || name.starts_with("\0");
    }

    bool path_exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    int create_file(std::string path) {
        std::filesystem::path p = path;

        if (path_exists(path)) {
            char buffer[2];
            info::warning("This file already exists. Do you want to overwrite it? (Y/N): ");
            ssize_t bytesRead = read(STDIN_FILENO, buffer, 1);
            if (bytesRead < 0) {
                std::string error = std::string("Failed to read input: ") + strerror(errno);
                info::error(error, errno);
                return errno;
            }
            buffer[bytesRead] = '\0';
            if (buffer[0] == 'n' || buffer[0] == 'N') return 0;
        }

        int fd = open(path.c_str(), O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            std::string error = std::string("Failed to create file: ") + strerror(errno);
            info::error(error, errno);
            return errno;
        }

        return 0;
    }

    int create_link(std::string path, std::string target, bool is_soft) {
        std::filesystem::path p = path;

        if (path_exists(path)) {
            char buffer[2];
            info::warning("The path already exists. Do you want to overwrite it? (Y/N): ");
            ssize_t bytesRead = read(STDIN_FILENO, buffer, 1);
            if (bytesRead < 0) {
                std::string error = std::string("Failed to read input: ") + strerror(errno);
                info::error(error, errno);
                return errno;
            }
            buffer[bytesRead] = '\0';
            if (buffer[0] == 'n' || buffer[0] == 'N') return 0;
        }

        if (is_soft) {
            if (symlink(target.c_str(), path.c_str()) != 0) {
                std::string error = std::string("Failed to create soft link: ") + strerror(errno);
                info::error(error, errno);
                return errno;
            }
        } else {
            if (link(target.c_str(), path.c_str()) != 0) {
                std::string error = std::string("Failed to create hard link: ") + strerror(errno);
                info::error(error, errno);
                return errno;
            }
        }

        return 0;
    }

    int create_fifo(std::string path) {
        std::filesystem::path p = path;

        if (path_exists(path)) {
            char buffer[2];
            info::warning("This FIFO already exists. Do you want to overwrite it? (Y/N): ");
            ssize_t bytesRead = read(STDIN_FILENO, buffer, 1);
            if (bytesRead < 0) {
                std::string error = std::string("Failed to read input: ") + strerror(errno);
                info::error(error, errno);
                return errno;
            }
            buffer[bytesRead] = '\0';
            if (buffer[0] == 'n' || buffer[0] == 'N') return 0;

            if (unlink(path.c_str()) != 0) {
                std::string error = std::string("Failed to remove existing fifo: ") + strerror(errno);
                info::error(error, errno);
                return errno;
            }
        }

        if (mkfifo(path.c_str(), 0666) != 0) {
            std::string error = std::string("Failed to create FIFO: ") + strerror(errno);
            info::error(error, errno);
            return errno;
        }

        return 0;
    }

public:
    Create() : Command("create", "Creates a new file", "") {}

    int exec(std::vector<std::string> args) {
        if (args.empty()) {
            io::print(
                "create: create a new file, link, or named pipe (fifo)\n"
                "usage: create [type] <name> [target]\n"
                "flags:\n"
                "-f | --file:          create a file (default)\n"
                "-s | --symbolic-link: create a symbolic link\n"
                "-h | --hard-link:     create a hard link\n"
                "-p | --fifo:          create a named pipe (fifo)\n"
            );
            return 0;
        }

        bool want_fifo = false;
        bool want_link = false;
        bool want_soft_link = false;
        bool want_file = true;

        std::vector<std::string> valid_args = {
            "-f", "-s", "-h", "-p",
            "--file", "--symbolic-link", "--hard-link", "--fifo"
        };

        if (args.size() < 1) {
            info::error("No name provided.");
            return -1;
        }

        std::string name;
        std::string target;

        for (size_t i = 0; i < args.size(); ++i) {
            const auto& a = args[i];

            if (!io::vecContains(valid_args, a) && a.starts_with("-")) {
                info::error("Invalid argument \"" + a + "\"\n");
                return -1;
            }

            if (a == "-f" || a == "--file") {
                want_file = true;
                want_link = false;
                want_fifo = false;
            } else if (a == "-s" || a == "--symbolic-link") {
                want_link = true;
                want_soft_link = true;
                want_file = false;
                want_fifo = false;
            } else if (a == "-h" || a == "--hard-link") {
                want_link = true;
                want_soft_link = false;
                want_file = false;
                want_fifo = false;
            } else if (a == "-p" || a == "--fifo") {
                want_fifo = true;
                want_file = false;
                want_link = false;
            } else if (name.empty()) {
                name = a;
            } else if (want_link && target.empty()) {
                target = a;
            }
        }

        if (name.empty()) {
            info::error("No name specified.");
            return -1;
        }

        if (want_file) {
            return create_file(name);
        } else if (want_link) {
            if (target.empty()) {
                info::error("No target specified for link.");
                return -1;
            }
            return create_link(name, target, want_soft_link);
        } else if (want_fifo) {
            return create_fifo(name);
        }

        info::error("No valid creation type specified.");
        return -1;
    }
};

int main(int argc, char* argv[]) {
    Create create;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    create.exec(args);
    return 0;
}