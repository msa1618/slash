
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../git/git.h"
#include "syntax_highlighting/cpp.h"
#include "syntax_highlighting/python.h"
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <unordered_set>
#include "../help_helper.h"

std::string tab_to_spaces(std::string content, int indent) {
  std::string spaces(indent, ' ');
  size_t pos = 0;
  while ((pos = content.find('\t', pos)) != std::string::npos) {
    content.replace(pos, 1, spaces);
    pos += spaces.length();
  }
  return content;
}

class Read {
  private:
    std::vector<std::pair<std::string, std::string>> filter_duplicates(std::vector<std::pair<std::string, std::string>> input) {
      std::unordered_set<std::string> seen;
      std::vector<std::pair<std::string, std::string>> result;

      for (const auto& p : input) {
          if (seen.insert(p.second).second) {
              result.push_back(p);
          }
      }
      return result;
    }

    int print_content(std::string fullpath, int tab_indent, bool hidden, bool git, bool reverse_lines, bool reverse_text, bool no_highlight, bool raw, bool filter_dups, bool sort,  bool specified_fromto, int from, int to) {
      std::vector<std::pair<std::string, std::string>> content_to_use;
      
      if(git) {
        GitRepo repo(std::filesystem::path(fullpath).parent_path());
        bool repo_exists = repo.get_repo() != nullptr;

        if(repo_exists) {
          std::vector<std::pair<std::string, std::string>> git_lines;

          std::filesystem::path file_path = std::filesystem::absolute(fullpath);
          std::filesystem::path root_path = std::filesystem::path(repo.get_root_path());
          std::string relative_path = std::filesystem::relative(file_path, root_path).string();

          git_lines = repo.get_file_changes(relative_path);
          content_to_use = git_lines;
        }
        else if(!repo_exists) {
          info::error("The directory of the file does not have a git repository.");
          return -1;
        }
      }
      else {
        auto rf = io::read_file(fullpath);
        if(!std::holds_alternative<std::string>(rf)) {
          std::string error = std::string("Failed to read file: ") + strerror(errno);
          info::error(error, errno);
          return errno;
        }

        std::string content = std::get<std::string>(rf);
        std::vector<std::pair<std::string, std::string>> raw_lines;

        std::vector<std::string> cnt = io::split(content, "\n");
        for(auto& line : cnt) raw_lines.push_back({"", line});
        content_to_use = raw_lines;
      }
      
      if(reverse_text) for(auto& [gc, line] : content_to_use) { std::reverse(line.begin(), line.end()); }
      if(reverse_lines) std::reverse(content_to_use.begin(), content_to_use.end());
      if(sort) std::sort(content_to_use.begin(), content_to_use.end(), [](std::pair<std::string, std::string> a, std::pair<std::string, std::string> b){
        return a.second < b.second;
      });
      if(filter_dups) content_to_use = filter_duplicates(content_to_use);

      if(raw) {
        for(auto [gc, line] : content_to_use) {
          io::print(tab_to_spaces(line + "\n", tab_indent));
        }
        return 0;
      }

      // From here, all formatting begins
      for(auto& [gc, content] : content_to_use) {
        if (hidden) {
          std::string space = cyan + "·" + reset;
          std::string enter = gray + "↲" + reset;

          std::string tab_dashes = [tab_indent](){
            std::string res;
            for(int i = 0; i < tab_indent - 2; i++) res += "─";
            return res;
          }();
          std::string tab = gray + "├" + tab_dashes + "┤" + reset;
          std::vector<std::string> control_pics = {
            "␀", "␁", "␂", "␃", "␄", "␅", "␆", "␇",
            "␈", "␉", "␊", "␋", "␌", "␍", "␎", "␏",
            "␐", "␑", "␒", "␓", "␔", "␕", "␖", "␗",
            "␘", "␙", "␚", "␛", "␜", "␝", "␞", "␟"
          };

          std::string line;
          for (auto &c: content) {
            int code = static_cast<int>(c);
            if (c == '\n') {
              line += control_pics[code] + enter;
              continue;
            }
            if (c == '\t') {
              line += tab;
              continue;
            }
            if (c == ' ') {
              line += space;
              continue;
            }
            if (code >= 0 && code < 32) {
              line += control_pics[code];
              continue;
            }
            line += c;
          }
          content = line;
        }
      }

      for(auto& [gc, l] : content_to_use) {
        // Highlighting isn't implemented yet when there are hidden characters, otherwise you'll see terminal gore
        if(!hidden && !reverse_text && !no_highlight) {
          if(fullpath.ends_with(".cpp")) l = cpp_sh(l);
          if(fullpath.ends_with(".py")) l = python_sh(l);
        } else break;
      }
      
      int line_width = 0;
      int longest_num_length = std::to_string(content_to_use.size() - 1).length(); // Line no of the last element

      line_width = std::to_string(content_to_use.size()).length();
      if(line_width % 2 == 0) line_width += 3;
      else line_width += 2;

      int i = 0;
      int idk = content_to_use.size();
      if(specified_fromto) {
        if(from != -1) i = from;
        if(to != -1) idk = to;
      }

      for (i; i < idk; i++) {
        if(!hidden) content_to_use[i].second = tab_to_spaces(content_to_use[i].second, tab_indent); // No need if hidden
        std::string line_no = std::to_string(i + 1);
        // Pad with spaces to align right, based on longest_num_length
        line_no.insert(0, std::string(longest_num_length - line_no.length(), ' '));

        // Print the line, with equal padding for line numbers
        io::print(std::string((line_width - 1) / 2, ' ') + " ");
        io::print(gray + line_no + reset + content_to_use[i].first);
        io::print(std::string((line_width - 1) / 2, ' ') + gray + "│ " + reset);

        // libgit2 adds a newline automatically for lines
        if(git) io::print(content_to_use[i].second);
        else io::print(content_to_use[i].second + "\n");
      }

      io::print("\n");
    }

  public:
    Read() {};

    int exec(std::vector<std::string> args) {
      if (args.empty()) {
        io::print(get_helpmsg({
					"Prints file content with syntax highlighting and various options",
					{
						"lynx <file>",
						"lynx [options] <file>"
					},
					{
						{"-r", "--reverse-lines", "Reverse lines when printing"},
						{"-R", "--reverse-text", "Reverse text when printing"},
						{"-g", "--git", "Print with git status for each line"},
						{"-h", "--hidden", "Print hidden characters"},
						{"-t", "--tab-size [n]", "Change tab size to n"},
						{"-u", "--unique", "Filter duplicate lines"},
						{"-s", "--sort", "Sort lines"},
						{"", "--from [n]", "Print lines starting from (n)"},
						{"", "--to [n]", "Print lines to (n)"},
						{"", "--no-highlight", "Print without highlighting"},
						{"", "--raw", "Print plain content only"},
					},
					{
						{"lynx main.cpp", "Print main.cpp's content"}, 
						{"lynx -t 4 script.ts", "Print script.ts but with tab size 4"},
						{"lynx -s mess", "Sort mess"}
					},
					"",
					""
				}));
        return 0;
      }


      std::vector<std::string> validArgs = {
        "-r", "-R", "-h", "-g", "-t", "-u", "-s",
        "--reverse-lines", "--reverse-text", "--hidden", "--git", "--tab-size", "--from", "--to", "--no-highlight", "--raw", "--unique", "--sort"
      };

      bool show_hidden_chars = false;
      bool reverse_lines     = false;
      bool reverse_text      = false;
      bool git               = false;
      bool no_highlight      = false;
      bool raw               = false;
      bool sort              = false;
      bool unique            = false;

      bool specified_fromto  = false;
      int from               = -1; // -1 means not specified here
      int to                 = -1;

      int indent             = 2;

      std::string path;
      for(int i = 0; i < args.size(); i++) {
        auto arg = args[i];
        if(!io::vecContains(validArgs, arg) && arg.starts_with("-")) {
          info::error("Invalid argument \"" + arg + "\".\n");
          return EINVAL;
        }

        if(!arg.starts_with("-")) {
          path = arg;
        }

        if(arg == "-h" || arg == "--hidden") show_hidden_chars = true;
        if(arg == "-r" || arg == "--reverse-lines") reverse_lines = true;
        if(arg == "-R" || arg == "--reverse-text") reverse_text = true;
        if(arg == "-g" || arg == "--git") git = true;
        if(arg == "-u" || arg == "--unique") unique = true;
        if(arg == "-s" || arg == "--sort") sort = true;
        if(arg == "--raw") raw = true;
        if(arg == "--no-highlight") no_highlight = true;
        if(arg == "--from") {
          if (i + 1 >= args.size()) {
            info::error("Missing from argument!");
            return -1;
          }
          try {
            from = std::stoi(args[i + 1]);
          } catch (...) {
            info::error("from must be a number!");
            return -1;
          }

          specified_fromto = true;
          i++; // Skip next arg
        }
        if(arg == "--to") {
          if (i + 1 >= args.size()) {
            info::error("Missing to argument!");
            return -1;
          }
          try {
            to = std::stoi(args[i + 1]);
          } catch (...) {
            info::error("to must be a number!");
            return -1;
          }

          specified_fromto = true;
          i++; // Skip next arg
        }
        if(arg == "-t") {
          if (i + 1 >= args.size()) {
            info::error("Missing tab size argument!");
            return -1;
          }
          try {
            indent = std::stoi(args[i + 1]);
          } catch (...) {
            info::error("Tab size must be a number!");
            return -1;
          }

          i++; // Skip next arg
        }
      }

      return print_content(path, indent, show_hidden_chars, git, reverse_lines, reverse_text, no_highlight, raw, unique, sort, specified_fromto, from, to);
    }
};

int main(int argc, char* argv[]) {
  Read read;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  read.exec(args);
  return 0;
}
