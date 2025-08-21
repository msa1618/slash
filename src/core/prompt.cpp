#include "prompt.h"

#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <boost/regex.hpp>
#include <iostream>  // For isprint
#include <cstring>   // For strerror
#include <optional>
#include "../abstractions/definitions.h"
#include "../git/git.h"
#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"
#include "../abstractions/json.h"
#include "../abstractions/json.hpp"
#include "../slash-utils/syntax_highlighting/helper.h"

#include "execution.h"

#include <sys/ioctl.h>
#include <grp.h>
#include "parser.h"
#include "../cmd_highlighter.h"
#include <algorithm>

#pragma region helpers

std::string rgb_to_ansi(std::array<int, 3> rgb, bool bg = false) {
  int r = rgb[0], g = rgb[1], b = rgb[2];
  if(r == 256 || g == 256 || b == 256) return "";
  std::stringstream res;
  if(bg) res << "\x1b[48;2;" << r << ";" << g << ";" << b << "m";
  else res << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
  return res.str();
}

bool is_ssh_server() {
  return getenv("SSH_CONNECTION") != nullptr || getenv("SSH_CLIENT") != nullptr || getenv("SSH_TTY") != nullptr;
}

std::string get_battery_interface_path() {
  std::vector <std::string> files;
  DIR *dir = opendir("/sys/class/power_supply/");
  if (!dir) return "";

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string name = entry->d_name;
    if (name == "AC" || name == "ACAD") continue;
    else if (name.starts_with("BAT") || name.starts_with("CMB")) {
      files.push_back(name);
    }
  }
  closedir(dir);

  if (files.empty()) return "";
  return files[0];
}

std::string get_prompt_config_path() {
    std::string home = getenv("HOME");
    auto settings = get_json(home + "/.slash/config/settings.json");
    if (settings.empty()) return get_prompt_config_path();

    auto prompt_path = get_string(settings, "pathOfPromptTheme");
    if (!prompt_path) return get_prompt_config_path();

    // If path is relative, prefix home
    if (prompt_path->starts_with("/")) return *prompt_path;
    return home + "/" + *prompt_path;
}

int get_terminal_width() {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
}

#pragma endregion

std::string get_time_segment() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if(j.empty()) {
    return "<failed>";
  }
  auto showSeconds = get_bool(j, "showSeconds", "time");
  auto use24hr = get_bool(j, "twentyfourhr", "time");
  auto foreground = get_int_array3(j, "foreground", "time");
  auto background = get_int_array3(j, "background", "time");
  auto bold = get_bool(j, "bold", "time").value_or(false);

  auto after = get_string(j, "after", "time");
  auto before = get_string(j, "before", "time");

  std::string ansi;
  if (foreground && *foreground != std::array<int, 3>{256, 256, 256}) {
    ansi += rgb_to_ansi(*foreground);
  }
  if (background && *background != std::array<int, 3>{256, 256, 256}) {
    ansi += rgb_to_ansi(*background, true);
  }
  if (bold) ansi = "\x1b[1m" + ansi;

  time_t t = time(nullptr);
  struct tm* now = localtime(&t);
  char buffer[16];

  if (showSeconds) {
    if (use24hr)
      strftime(buffer, sizeof(buffer), "%H:%M:%S", now);
    else
      strftime(buffer, sizeof(buffer), "%I:%M:%S %p", now);
  } else {
    if (use24hr)
      strftime(buffer, sizeof(buffer), "%H:%M", now);
    else
      strftime(buffer, sizeof(buffer), "%I:%M %p", now);
  }

  std::stringstream res;
  res << ansi << *before << std::string(buffer) << *after << reset;

  return res.str();
}

std::string get_user_segment() {
    std::string home = getenv("HOME");
    auto j = get_json(get_prompt_config_path());
    if (j.empty()) return "";

    auto enabled = get_bool(j, "enabled", "user");
    if (!enabled || !*enabled) return "";

    std::string user = getenv("USER") ?: "";
    auto after = get_string(j, "after", "user").value_or("");
    auto before = get_string(j, "before", "user").value_or("");
    auto foreground = get_int_array3(j, "foreground", "user");
    auto background = get_int_array3(j, "background", "user");
    auto bold = get_bool(j, "bold", "user").value_or(false);

    std::string ansi;
    if (foreground && *foreground != std::array<int, 3>{256, 256, 256}) {
        ansi += rgb_to_ansi(*foreground);
    }
    if (background && *background != std::array<int, 3>{256, 256, 256}) {
        ansi += rgb_to_ansi(*background, true); // true for background
    }
    if (bold) ansi = "\x1b[1m" + ansi;

    return ansi + before + user + after + reset;
}


std::string get_group_segment() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return "";

  auto enabled = get_bool(j, "enabled", "group");
  if (!enabled || !*enabled) return "";

  gid_t gid = getgid();
  struct group *grp = getgrgid(gid);
  std::string group = grp ? grp->gr_name : "";

  auto after = get_string(j, "after", "group").value_or("");
  auto before = get_string(j, "before", "group").value_or("");
  auto foreground = get_int_array3(j, "foreground", "group");
  auto background = get_int_array3(j, "background", "group");
  auto bold = get_bool(j, "bold", "group").value_or(false);

  std::string ansi;
  if (foreground && *foreground != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*foreground);
  if (background && *background != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*background, true);

  if (bold) ansi = "\x1b[1m" + ansi;

  return ansi + before + group + after + reset;
}

std::string get_hostname_segment() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return "";

  auto enabled = get_bool(j, "enabled", "hostname");
  if (!enabled || !*enabled) return "";

  char hostname[256];
  gethostname(hostname, sizeof(hostname));

  auto after = get_string(j, "after", "hostname").value_or("");
  auto before = get_string(j, "before", "hostname").value_or("");
  auto foreground = get_int_array3(j, "foreground", "hostname");
  auto background = get_int_array3(j, "background", "hostname");
  auto bold = get_bool(j, "bold", "hostname").value_or(false);

  std::string ansi;
  if (foreground && *foreground != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*foreground);
  if (background && *background != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*background, true);

  if (bold) ansi = "\x1b[1m" + ansi;

  return ansi + before + std::string(hostname) + after + reset;
}

std::string get_ssh_segment() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return "";

  auto enabled = get_bool(j, "enabled", "ssh");
  if (!enabled || !*enabled) return "";

  if (!is_ssh_server()) return "";

  auto after = get_string(j, "after", "ssh").value_or("");
  auto before = get_string(j, "before", "ssh").value_or("");
  auto text = get_string(j, "text", "ssh").value_or("ssh");
  auto foreground = get_int_array3(j, "foreground", "ssh");
  auto background = get_int_array3(j, "background", "ssh");
  auto bold = get_bool(j, "bold", "ssh").value_or(false);

  std::string ansi;
  if (foreground && *foreground != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*foreground);
  if (background && *background != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*background, true);

  if (bold) ansi = "\x1b[1m" + ansi;

  return ansi + before + text + after + reset;
}

std::string get_git_segment() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return "";

  auto enabled = get_bool(j, "enabled", "git-branch");
  if (!enabled || !*enabled) return "";

  char cwd_buffer[512];
  std::string cwd = getcwd(cwd_buffer, sizeof(cwd_buffer));
  GitRepo repo(cwd);
  std::string branch = repo.get_branch_name();
  if (branch.empty()) return "";

  auto after = get_string(j, "after", "git-branch").value_or("");
  auto before = get_string(j, "before", "git-branch").value_or("");
  auto foreground = get_int_array3(j, "foreground", "git-branch");
  auto background = get_int_array3(j, "background", "git-branch");
  auto bold = get_bool(j, "bold", "git-branch").value_or(false);

  std::string ansi;
  if (foreground && *foreground != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*foreground);
  if (background && *background != std::array<int, 3>{256, 256, 256})
    ansi += rgb_to_ansi(*background, true);

  if (bold) ansi = "\x1b[1m" + ansi;

  return ansi + before + branch + after + reset;
}

std::string get_cwd_segment() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return "";

  auto enabled = get_bool(j, "enabled", "currentdir");
  if (!enabled || !*enabled) return "";

  auto foreground = get_int_array3(j, "foreground", "currentdir");
  auto background = get_int_array3(j, "background", "currentdir");
  auto before = get_string(j, "before", "currentdir").value_or("");
  auto after = get_string(j, "after", "currentdir").value_or("");
  auto bold = get_bool(j, "bold", "currentdir").value_or(false);

  char buffer[512];
  std::string cwd = getcwd(buffer, sizeof(buffer));
  std::string home_str = getenv("HOME");

  if (cwd.starts_with(home_str))
    cwd.replace(0, home_str.size(), "~");

  std::string ansi;
  if (foreground && *foreground != std::array<int, 3>{256, 256, 256}) ansi += rgb_to_ansi(*foreground);
  if (background && *background != std::array<int, 3>{256, 256, 256}) ansi += rgb_to_ansi(*background, true);

  if (bold) ansi = "\x1b[1m" + ansi;

  return ansi + before + cwd + after + reset;
}

std::string get_prompt_segment() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return "";

  auto enabled = get_bool(j, "enabled", "prompt");
  if (!enabled || !*enabled) return "";

  auto foreground = get_int_array3(j, "foreground", "prompt");
  auto background = get_int_array3(j, "background", "prompt");
  auto after = get_string(j, "after", "prompt").value_or("");
  auto before = get_string(j, "before", "prompt").value_or("");
  auto character = get_string(j, "character", "prompt").value_or(">");
  auto bold = get_bool(j, "bold", "prompt").value_or(false);
  auto newlineBefore = get_bool(j, "newlineBefore", "prompt").value_or(false);
  auto newlineAfter = get_bool(j, "newlineAfter", "prompt").value_or(false);

  std::string ansi = "";
  if (foreground && *foreground != std::array<int, 3>{256, 256, 256}) {
    ansi += rgb_to_ansi(*foreground);
  }
  if(background && *background != std::array<int, 3>{256, 256, 256}) {
    ansi += rgb_to_ansi(*background, true);
  }
  if (bold) ansi = "\x1b[1m" + ansi;

  std::stringstream out;

  if (newlineBefore) out << "\n" << "\x1b[2K";
  out << ansi << before << character << after << reset;
  if (newlineAfter) out << "\n";


  return out.str();
}

void draw_prompt() {
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return;
  std::stringstream prompt;

  auto time_enabled = get_bool(j, "enabled", "time");
  auto align_right = get_bool(j, "alignRight", "time");

  if (time_enabled && *time_enabled) {
    if (!align_right && *align_right) {
        prompt << get_time_segment();
    }
  }

  if (get_bool(j, "enabled", "user")) prompt << get_user_segment();
  if (get_bool(j, "enabled", "group")) prompt << get_group_segment();
  if (get_bool(j, "enabled", "hostname")) prompt << get_hostname_segment();
  if (get_bool(j, "enabled", "currentdir")) prompt << get_cwd_segment();
  if (get_bool(j, "enabled", "git-branch")) prompt << get_git_segment();
  if (get_bool(j, "enabled", "ssh")) { prompt << get_ssh_segment(); }

  io::print(prompt.str());

  if (time_enabled && *time_enabled) {
    if (align_right && *align_right) {
        io::print_right(get_time_segment());
    }
  }

  if (get_bool(j, "enabled", "prompt") == true) {
    io::print(get_prompt_segment());
  }
}

void redraw_prompt(std::string content, int char_pos = -1) { // -1: not specified
  std::string home = getenv("HOME");
  auto j = get_json(get_prompt_config_path());
  if (j.empty()) return;

  auto newline_before = get_bool(j, "newlineBefore", "prompt");
  auto prompt_chars = get_string(j, "character", "prompt");

  if(newline_before && *newline_before) {
    int content_rows = (int)ceil(((double)io::strip_ansi(content).length() + io::strip_ansi(prompt_chars.value_or("")).length()) / get_terminal_width());
    if(content_rows == 0) {
      io::print("\r");
      io::print(get_prompt_segment() + content);
    } else {
      std::stringstream move_cursor;
      move_cursor << "\x1b[" << content_rows << "A";
      io::print(move_cursor.str());
      io::print("\r" + get_prompt_segment() + content);
    }

    if(char_pos != -1) {
      for(int i = 0; i < (int)io::strip_ansi(content).size() - char_pos; i++) {
        io::print("\x1b[1D");
      }
    }
  } else {
    io::print("\x1b[2K\r");
    draw_prompt();
  }
}


std::variant<std::string, int> read_input(int& history_index) {
  std::string buffer = "";
  std::string backup_buffer = buffer; // To retrieve the command when browsing history if nothing was typed
  int char_pos = 0;
  history_index = -1;
  char c = 0;

  while(true) {
    if(read(STDIN_FILENO, &c, 1) != 1) continue;

    if(c == '\n' || c == '\r') {
      io::print("\n");
      break;
    }

    if (c == 127 || c == 8) {
      if (char_pos > 0) {
        char_pos--;
        buffer.erase(char_pos, 1);

        redraw_prompt(highl(buffer), char_pos);
        backup_buffer = buffer;
      }
      continue;
    }

    if(c == 27) { // Escape sequence
      char seq[10];
      int n = 0;

      while (n < 9) {
          int r = read(STDIN_FILENO, &seq[n], 1);
          if (r != 1) break;
          n++;
          // Escape sequences usually end with a letter
          if ((seq[n-1] >= 'A' && seq[n-1] <= 'Z') || (seq[n-1] >= 'a' && seq[n-1] <= 'z')) break;
      }

      std::string seq_str(seq, n);

      if (seq_str == "[A") {
          const char *home = getenv("HOME");
          auto history = io::read_file(std::string(home) + "/.slash/.slash_history");
          if (std::holds_alternative<int>(history)) {
              std::string err = std::string("Failed to fetch history: ") + strerror(errno);
              info::error(err, errno, "~/.slash/.slash_history");
              return errno;
          }
          std::vector<std::string> commands = io::split(std::get<std::string>(history), "\n");
          while (!commands.empty() && commands.back().empty()) commands.pop_back();
          if (commands.empty()) continue;

          if (history_index == -1) {
              backup_buffer = buffer; // save what was typed
              history_index = 0;       // jump into newest history
          } else if (history_index + 1 < (int)commands.size()) {
              history_index++;
          }

          std::string command = commands[commands.size() - 1 - history_index];
          redraw_prompt(highl(command));
          char_pos = (int)command.length();
          buffer = command;
} else if (seq_str == "[B") { // Down arrow
        const char *home = getenv("HOME");
        auto history = io::read_file(std::string(home) + "/.slash/.slash_history");
        if (std::holds_alternative<int>(history)) {
            std::string err = std::string("Failed to fetch history: ") + strerror(errno);
            info::error(err, errno, "~/.slash/.slash_history");
            return errno;
        }
        std::vector<std::string> commands = io::split(std::get<std::string>(history), "\n");
        if (commands.empty()) continue;
        while(commands.back() == "\n" || commands.back().empty()) commands.pop_back();

        if (history_index < commands.size() && history_index > -1)
            history_index--;
        else redraw_prompt(highl(buffer));

        std::string command;
        if(history_index > -1) {
          command = commands[commands.size() - history_index];
        } else command = backup_buffer;
        redraw_prompt(highl(command));
        char_pos = (int)command.length();
        buffer = command;

    } else if (seq_str == "[C") { // Right arrow
        if (char_pos < (int)buffer.size()) {
            io::print("\x1b[C");
            char_pos++;
        }
    } else if (seq_str == "[D") { // Left arrow
        if (char_pos > 0) {
            io::print("\x1b[D");
            char_pos--;
        }
    } else if(seq_str == "[1;5C" || seq_str == "[1;2C") { // Ctrl+Right and Shift+Right
        if(char_pos >= (int)buffer.size()) continue;
        int new_pos = char_pos;
        // Move right until space or end of buffer
        while(new_pos < (int)buffer.size() && !isspace(buffer[new_pos])) {
            new_pos++;
        }
        // Move one more to skip spaces if any (optional, depends on desired behavior)
        while(new_pos < (int)buffer.size() && isspace(buffer[new_pos])) {
            new_pos++;
        }
        int move = new_pos - char_pos;
        if (move > 0) {
            std::stringstream ss;
            ss << "\x1b[" << move << "C";
            io::print(ss.str());
            char_pos = new_pos;
        }
    } else if(seq_str == "[1;5D" || seq_str == "[1;2D") { // Ctrl+Left and Shift+Left
        if(char_pos <= 0) continue;
        int new_pos = char_pos;
        // Move left skipping spaces first
        while(new_pos > 0 && isspace(buffer[new_pos - 1])) {
            new_pos--;
        }
        // Move left until space or start of buffer
        while(new_pos > 0 && !isspace(buffer[new_pos - 1])) {
            new_pos--;
        }
        int move = char_pos - new_pos;
        if (move > 0) {
            std::stringstream ss;
            ss << "\x1b[" << move << "D";
            io::print(ss.str());
            char_pos = new_pos;
        }
    } else if(seq_str == "[1;3D") {
      std::stringstream ss;
      ss << "\x1b[3G";
      io::print(ss.str());

      char_pos = 0;
    }

      continue;
    }

    if(isprint(static_cast<unsigned char>(c))) {
    if(c == '\'' || c == '"' || c == '(' || c == '{' || c == '[') {
        buffer.insert(buffer.begin() + char_pos, c);      // insert first quote
        char_pos++;

        char second = 0;
        if (c == '\'')  second = '\'';
        if (c == '"')   second = '"';
        if (c == '(')   second = ')';
        if (c == '{')   second = '}';
        if (c == '[')   second = ']';

        if(second == 0) continue;
        
        buffer.insert(buffer.begin() + char_pos, second);

        // Clear line and reprint everything with syntax highlight
        redraw_prompt(highl(buffer));

        // Move cursor back to position inside quotes
        int cursor_back = buffer.length() - char_pos;
        if (cursor_back > 0) {
            std::stringstream move_left;
            move_left << "\x1b[" << cursor_back << "D";
            io::print(move_left.str());
        }
    } else {
        buffer.insert(buffer.begin() + char_pos, c);
        char_pos++;

        redraw_prompt(highl(buffer));

        int cursor_back = buffer.length() - char_pos;
        if (cursor_back > 0) {
            std::stringstream move_left;
            move_left << "\x1b[" << cursor_back << "D";
            io::print(move_left.str());
        }
    }
    backup_buffer = buffer;
}
  }
  return buffer;
}


std::string print_prompt(nlohmann::json& j) {
  char buffer[512];
  std::string cwd = getcwd(buffer, sizeof(buffer));
  std::stringstream prompt;

  if (j.empty()) return "";

  draw_prompt();

  int history_index = 0;
  auto input = read_input(history_index);
  if (!std::holds_alternative<std::string>(input)) {
    std::string error = std::string("Failed to fetch history: ") + strerror(errno);
    info::error(error, errno);
    return "";
  }

  std::string input_str = std::get<std::string>(input);
  return input_str;
}

