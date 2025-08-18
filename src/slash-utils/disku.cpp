#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cmath>
#include <filesystem> // Just for the extension
#include <vector>
#include <string>
#include <sys/statvfs.h>

#include "../abstractions/info.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/definitions.h"
#include "../help_helper.h"
#include <algorithm>

class Disku {
  private:
    std::string get_type(std::string name) {
      std::vector<std::string> videos = {
          ".mp4", ".mkv", ".mov", ".mpg", ".mpeg", ".avi", ".flv", ".wmv", ".webm", ".vob",
          ".ogv", ".mts", ".m2ts", ".3gp", ".3g2", ".rm", ".rmvb", ".divx", ".xvid",
          ".f4v", ".m4v", ".asf", ".mxf", ".roq", ".nsv"
      };

      std::vector<std::string> images = {
          ".png", ".jpg", ".jpeg", ".webp", ".bmp", ".gif", ".svg", ".raw", ".tiff", ".tif",
          ".heic", ".ico", ".jfif", ".exif", ".avif", ".eps", ".psd", ".cdr", ".indd",
          ".ai"
      };

      std::vector<std::string> documents = {
          ".pdf", ".doc", ".docx", ".txt", ".odt", ".rtf", ".xls", ".xlsx", ".ppt", ".pptx",
          ".csv", ".tsv", ".xml", ".json", ".yaml", ".yml", ".md", ".epub", ".azw", ".azw3",
          ".djvu", ".pages", ".log", ".tex", ".wpd", ".wps", ".msg", ".eml", ".ics", ".vcf",
          ".odp", ".ods", ".potx", ".xltx", ".dotx"
      };

      std::vector<std::string> audio = {
          ".mp3", ".wav", ".aac", ".flac", ".ogg", ".m4a", ".wma", ".alac", ".aiff", ".opus",
          ".aif", ".aifc", ".caf", ".amr", ".3gp", ".3g2", ".mid", ".midi", ".rmi", ".mka",
          ".spx", ".tta", ".ac3", ".dts", ".cda"
      };

      std::vector<std::string> archives = {
          ".zip", ".rar", ".7z", ".tar", ".gz", ".bz2", ".xz", ".iso", ".pea", ".lzma",
          ".cab", ".arj", ".z", ".lz", ".lzh", ".txz", ".tlz", ".apk", ".jar", ".war",
          ".ear", ".cpio", ".rpm", ".deb", ".dmg", ".vhd", ".vmdk", ".ova"
      };
      std::vector<std::string> code = {
        ".cpp", ".c", ".h", ".hpp", ".py", ".java", ".js", ".ts", ".html", ".css",
        ".php", ".rb", ".go", ".rs", ".swift", ".kt", ".sh", ".bat", ".sql", ".r",
        ".c++", ".cc", ".asm", ".m", ".mm", ".cs", ".scala", ".pl", ".pm", ".lua",
        ".groovy", ".erl", ".ex", ".exs", ".fs", ".fsi", ".fsx", ".vb", ".vbs",
        ".ps1", ".psm1", ".dart", ".clj", ".cljs", ".edn", ".jl", ".lisp", ".cl",
        ".scm", ".ss", ".sml", ".ml", ".mli", ".pas", ".pp", ".adb", ".ads", ".for",
        ".f90", ".f95", ".f03", ".f08", ".hx", ".hxsl", ".hlsl", ".glsl", ".metal",
        ".wgsl", ".cu", ".cuh", ".d", ".vala", ".vapi", ".vhd", ".vhdl", ".sv", ".v",
        ".svh", ".verilog", ".tcl", ".hx", ".json", ".yml", ".yaml", ".toml",
        ".ini", ".conf", ".cfg", ".makefile", ".mk", ".cmake", ".dockerfile", ".gradle",
        ".pom", ".pro", ".sbt", ".gemspec", ".rake", ".swiftpm", ".xcodeproj",
        ".xaml", ".xib", ".storyboard", ".plist", ".ipynb", ".tsx", ".jsx", ".coffee",
        ".iced", ".nim", ".pascal", ".ada", ".asm", ".S", ".s", ".rs", ".sol", ".v",
        ".sv", ".svh", ".vhd", ".vhdl", ".thy", ".tla", ".bzl", ".bazel", ".st",
        ".ssc", ".robot", ".feature", ".go", ".nim", ".moon", ".pyx", ".pxd", ".pxi",
        ".sql", ".tcl", ".v", ".w", ".wl", ".wlt", ".xl", ".xls", ".xsl", ".xsd",
        ".xslt", ".xquery", ".zsh", ".fish", ".awk", ".sed", ".make", ".mak", ".b",
        ".bat", ".cmd", ".ps1", ".psm1", ".dts", ".dtsi", ".rkt", ".ss", ".scm", ".sls",
        ".sps", ".sas", ".sml", ".fun", ".ts", ".tsx", ".v", ".sv", ".svh", ".vhdl"
    };

      std::string result;
      std::filesystem::path path = name;
      if(io::vecContains(videos, path.extension())) result = "vid";
      else if(io::vecContains(images, path.extension())) result = "img";
      else if(io::vecContains(documents, path.extension())) result = "doc";
      else if(io::vecContains(code, path.extension())) result = "code";
      else if(io::vecContains(audio, path.extension())) result = "audio";
      else if(io::vecContains(archives, path.extension())) result = "arch";
      else result = "other";

      return result;
    }

    unsigned long long get_disk_storage(std::string path) {
      struct statvfs st;
      if (statvfs(path.c_str(), &st) != 0) {
        std::string error = std::string("Failed to stat disk: ") + strerror(errno);
        info::error(error, errno);
        return -1;
      }

      return st.f_blocks * st.f_frsize;
    }

    std::vector<std::vector<int>> get_dir_usages(std::string dirpath, bool no_dirs) {
      DIR* dir = opendir(dirpath.c_str());
      if(dir == nullptr) {
        std::string error = std::string("Failed to open dir: ") + strerror(errno);
        info::error(error, errno);
        return {};
      }

      std::vector<int> videos;
      std::vector<int> audios;
      std::vector<int> images;
      std::vector<int> documents;
      std::vector<int> code;
      std::vector<int> archive;
      std::vector<int> other;


      struct dirent* entry;
      while((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        std::string type = get_type(name);
        std::string full_path = dirpath + "/" + name;

        if(name == "." || name == "..") continue;

        struct stat st;
        if (stat(full_path.c_str(), &st) == -1) {
          std::string error = "Failed to stat: " + full_path + " (" + strerror(errno) + ")";
          info::error(error, errno);
          continue;
        }

        if(S_ISDIR(st.st_mode)) {
          continue;
        }

        if(S_ISREG(st.st_mode)) {
          std::string type = get_type(name);
          int size = st.st_size;
          if (type == "vid") videos.push_back(size);
          else if (type == "audio") audios.push_back(size);
          else if (type == "img") images.push_back(size);
          else if (type == "doc") documents.push_back(size);
          else if (type == "code") code.push_back(size);
          else if (type == "arch") archive.push_back(size);
          else other.push_back(size);
        }

        if(S_ISLNK(st.st_mode)) {
          std::vector<char> buf(1024);
          ssize_t len = readlink(full_path.c_str(), buf.data(), buf.size() - 1);
          if (len == -1) {
            std::string error = std::string("Failed to get symlink target: ") + strerror(errno);
            info::error(error, errno);
            return {};
          }
          buf[len] = '\0';
          std::string target = buf.data();
          std::string type = get_type(name);
          int size = st.st_size;
          if (type == "vid") videos.push_back(size);
          else if (type == "audio") audios.push_back(size);
          else if (type == "img") images.push_back(size);
          else if (type == "doc") documents.push_back(size);
          else if (type == "code") code.push_back(size);
          else if (type == "arch") archive.push_back(size);
          else other.push_back(size);
        }
      }

      return {
        videos, audios, images,
        documents, code, archive, other
      };
    }

    unsigned long long get_size(std::string path) {
        struct stat st;
        if (lstat(path.c_str(), &st) < 0) return 0;

        if (!S_ISDIR(st.st_mode)) {
            // It's a file or link: return its size
            return st.st_size;
        }

        int total = 0;
        DIR *dir = opendir(path.c_str());
        if (!dir) return 0;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;

            std::string fullpath = path + "/" + entry->d_name;
            total += get_size(fullpath);
        }
        closedir(dir);
        return total;
    }

    std::string to_human_readable(uint64_t size) {
      uint64_t divisor = 1;
      std::string prefix;

      if (size < 1024) {
          divisor = 1;
          prefix = "B";
      } else if (size < 1024 * 1024) {
          divisor = 1024;
          prefix = "KiB";
      } else if (size < 1024 * 1024 * 1024) {
          divisor = 1024 * 1024;
          prefix = "MiB";
      } else if (size < 1024ULL * 1024 * 1024 * 1024) {
          divisor = 1024ULL * 1024 * 1024;
          prefix = "GiB";
      } else if (size < 1024ULL * 1024 * 1024 * 1024 * 1024) {
          divisor = 1024ULL * 1024 * 1024 * 1024;
          prefix = "TiB";
      }

      return std::to_string(size / divisor) + prefix;
    }

  void print_table(std::string dirpath, bool no_dirs) {
    DIR* d = opendir(dirpath.c_str());
    if (d == nullptr) {
      std::string error = std::string("Failed to open dir: ") + strerror(errno);
      return;
    }

    struct dirent* entry;
    std::vector<std::tuple<std::string, std::string, int, std::string>> entries;

    while ((entry = readdir(d)) != nullptr) {
      std::string name = entry->d_name;
      if (name == "." || name == "..") continue;

      std::string type;
      std::string fullpath = dirpath + "/" + name;

      std::string filetype = get_type(name);
      std::string color;
      if (filetype == "vid") color = magenta;
      else if (filetype == "audio") color = cyan;
      else if (filetype == "img") color = yellow;
      else if (filetype == "doc") color = green;
      else if (filetype == "code") color = blue;
      else if (filetype == "arch") color = red;
      else color = gray;

      switch (entry->d_type) {
        case DT_DIR: {
          if(no_dirs) continue;
          type = "dir"; color = bold + white;
          break;
        }
        default: type = "file"; break;
      }

      struct stat st;
      if (stat(fullpath.c_str(), &st) == -1) {
        std::string error = std::string("Failed to stat file: ") + strerror(errno);
        info::error(error, errno);
        return;
      }
      int size = st.st_size;

      entries.push_back(std::make_tuple(name, type, size, color));
    }

    int longest_name_length = 0;
    int longest_num_length = 0;

    for (int i = 0; i < entries.size(); i++) {
      if (std::to_string(i).length() > longest_num_length)
        longest_num_length = std::to_string(i).length();
      if (std::get<0>(entries[i]).length() > longest_name_length)
        longest_name_length = std::get<0>(entries[i]).length();
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      bool a_is_dir = std::get<1>(a) == "dir";
      bool b_is_dir = std::get<1>(b) == "dir";

      if (a_is_dir != b_is_dir) {
        return a_is_dir;
      }

      return std::get<0>(a) < std::get<0>(b); // sort by name after dirs
    });

    for (int i = 0; i < entries.size(); i++) {
      std::string num = std::to_string(i);
      std::string color = std::get<3>(entries[i]);
      std::string name = color + std::get<0>(entries[i]) + reset;
      std::string fullpath = dirpath + "/" + std::get<0>(entries[i]);

      num.resize(longest_num_length, ' ');
      name.resize(longest_name_length + color.length() + reset.length(), ' '); // adjust for color codes

      io::print(num + " │ " + name + " │ " + to_human_readable(get_size(fullpath)) + "\n");
    }
  }

  void print_usage(std::string dirpath, bool no_dirs) {
    auto usages = get_dir_usages(dirpath, no_dirs);
    std::string total_usage = to_human_readable(get_size(dirpath));

    auto sum_sizes = [](const std::vector<int>& vec) {
      int size = 0;
      for(auto& s : vec) size += s;
      return size;
    };

    int video_size = sum_sizes(usages[0]);
    int audio_size = sum_sizes(usages[1]);
    int image_size = sum_sizes(usages[2]);
    int document_size = sum_sizes(usages[3]);
    int code_size = sum_sizes(usages[4]);
    int archive_size = sum_sizes(usages[5]);
    int other_size = sum_sizes(usages[6]);

    std::vector<std::tuple<int, std::string, std::string>> all_sizes = {
      {video_size, bg_magenta, "Video"},
      {audio_size, bg_cyan, "Audio"},
      {image_size, bg_yellow, "Image"},
      {document_size, bg_green, "Document"},
      {code_size, bg_blue, "Code"},
      {archive_size, bg_red, "Archives"},
      {other_size, bg_gray, "Other"}
    };

    std::sort(all_sizes.begin(), all_sizes.end(), [](const auto& a, const auto& b) {
      return std::get<0>(a) < std::get<0>(b);
    });

    for (size_t i = 0; i < all_sizes.size(); ++i) {
      int size = std::get<0>(all_sizes[i]);
      std::string color = std::get<1>(all_sizes[i]);
      std::string label = std::get<2>(all_sizes[i]);

      io::print(color + "  " + reset);
      io::print(" " + label + ": " + to_human_readable(size) + "   ");
    }

    int total_size = get_size(dirpath);
    float percent_used = ((float)total_size / get_disk_storage("/")) * 100;

    io::print("\n\n");
    io::print("Total usage: ");
    io::print(cyan + to_human_readable(get_size(dirpath)) + reset + "\n");

    io::print("Disk % used: ");
    std::stringstream ss;
    ss << cyan << std::setprecision(5) << std::to_string(percent_used) << "%" << reset;
    io::print(ss.str());

    io::print("\n\n");

    print_table(dirpath, no_dirs);
  }


  public:
    Disku() {}

    int exec(std::vector<std::string> args) {
      std::string path;
      if(args.empty()) {
        char buffer[PATH_MAX];
        if(!getcwd(buffer, PATH_MAX)) {
          std::string error = std::string("Failed to get current directory: ") + strerror(errno);
          info::error(error, errno);
          return errno;
        }
        path = buffer;
      }

      std::vector<std::string> validArgs = {
        "--no-dirs"
      };

      bool no_dirs = false;

      for(auto& arg : args) {
        if(io::vecContains(validArgs, arg)) {
          info::error("Invalid argument \"" + arg + "\"!");
          return EINVAL;
        }
        if(arg == "--no-dirs") {
          no_dirs = true;
        }
        if(arg == "-h" || arg == "--help") {
          io::print(get_helpmsg({
            "Prints the disk usage of each file and directory",
            {
              "disku",
              "disku [option]"
            },
            {
              {"", "--no-dirs", "Do not include directories"},
              {"-h", "--help", "Show this help message"}
            },
            {
              {"disku", "Show disk usage of each file and directory"},
              {"disku --no-dirs", "Show disk usage of each file only"}
            },
            "",
            ""
          }));
          return 0;
        }
        if(!arg.starts_with("-")) {
          path = arg;
        }
      }
      print_usage(path, no_dirs);
      return 0;
    }
};

int main(int argc, char* argv[]) {
  Disku disku;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  disku.exec(args);
  return 0;
}
