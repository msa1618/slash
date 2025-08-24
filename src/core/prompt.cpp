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
#include "jobs.h"
#include "startup.h"

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

struct SegmentStyle {
  std::string begin;
  std::string end;
};

SegmentStyle get_segment_style(std::string style, bool first, bool last) {
  if(style == "rectangle") return {" ", " "};
  if(style == "arrow") return {"", ""};
  if(style == "chip") return {"", ""};
  if(style == "chip-arrow") {
    if(first) return {"", ""};
    if(last) return {" ", ""};
    return {"", ""};
  }

  if(style == "chip-rectangle") {
    if(first) return {"", " "};
    if(last) return {"", ""};
    return {"", ""};
  }
}

// Declared here to solve circular dependency
std::string get_jobs_segment(bool only_want_unused_or_not = false);
std::string get_ssh_segment(bool only_want_unused_or_not = false);
std::string get_git_segment(bool only_want_unused_or_not = false);

std::vector<std::string> get_order() {
  std::vector<std::string> default_order = {
      "before-all", "time", "user", "group", "hostname",
      "currentdir", "git-branch", "jobs", "ssh"
  };

  auto j = get_json(get_prompt_config_path());
  if(j.empty()) return default_order;

    std::vector<std::string> order;
    if (j.contains("order") && j["order"].is_array()) {
        for (auto& elm : j["order"]) {
            if (elm.is_string()) order.push_back(elm.get<std::string>());
        }
    } else {
        order = default_order;
    }

    order.erase(std::remove_if(order.begin(), order.end(), [j](std::string s){
      return !get_bool(j, "enabled", s).value_or(false);
    }));

    for(auto& seg : order) {
      if(seg == "git-branch" && get_git_segment(true) == "<UNUSED>") {
        order.erase(std::remove_if(order.begin(), order.end(), [](std::string& s) {
          return s == "git-branch";
        }));
      }

      if(seg == "ssh" && get_ssh_segment(true) == "<UNUSED>") {
        order.erase(std::remove_if(order.begin(), order.end(), [](std::string& s) {
          return s == "ssh";
        }));
      }

      if(seg == "jobs" && get_jobs_segment(true) == "<UNUSED>") {
        order.erase(std::remove_if(order.begin(), order.end(), [](std::string& s) {
          return s == "jobs";
        }));
      }
    }

    return order;
}

std::string print_segment(std::string seg_name, std::string content, std::string after, std::string before, bool bold, std::string nerd_icon, std::array<int, 3> fg, std::array<int, 3> bg,  SegmentStyle style, std::string style_name, bool next_segment_unused) {
  auto j = get_json(get_prompt_config_path());
  if(j.empty()) return "";

  std::string bg_as_fg = rgb_to_ansi(bg, false);
  std::string fg_as_bg = rgb_to_ansi(fg, true);

  auto padding_enabled = get_bool(j, "enabled", "padding").value_or(true);
  auto padding_spaces = get_int(j, "spaces", "padding").value_or(1);

  std::stringstream ss;
  if(style_name == "chip" || style_name == "chip-rectangle" || style_name == "rectangle") {
    ss << before
       << bg_as_fg << style.begin << reset
       << rgb_to_ansi(fg) << rgb_to_ansi(bg, true);
    if(padding_enabled && padding_spaces > 0) ss << std::string(padding_spaces, ' ');
    ss << (bold ? "\x1b[1m" : "") << nerd_icon << content;
    if(padding_enabled && padding_spaces > 0) ss << std::string(padding_spaces, ' ');
    ss << reset
       << bg_as_fg << style.end << reset
       << after;

  } else if(style_name == "arrow") {
    auto order = get_order();
    auto it = std::find(order.begin(), order.end(), seg_name);

    std::array<int, 3> next_seg_bg = {256, 256, 256};

    if (it != order.end() && std::next(it) != order.end()) {
        std::string next_seg = *std::next(it);
        next_seg_bg = get_int_array3(j, "background", next_seg).value_or(std::array<int,3>{256, 256, 256});
    }

    ss << before
       << rgb_to_ansi(fg) << rgb_to_ansi(bg, true);
    if(padding_enabled && padding_spaces > 0) ss << std::string(padding_spaces, ' ');
    ss << (bold ? "\x1b[1m" : "") << nerd_icon << content;
    if(padding_enabled && padding_spaces > 0) ss << std::string(padding_spaces, ' ');
    ss << reset
       << bg_as_fg;
    if(!next_segment_unused) ss << rgb_to_ansi(next_seg_bg, true);
    ss << style.end << reset;
  } else {
      ss << before << rgb_to_ansi(fg) << rgb_to_ansi(bg, true) << (bold ? "\x1b[1m" : "") << nerd_icon << content << after << reset;
  }

  return ss.str();
}

bool is_next_segment_unused(std::string seg_name) {
    auto order = get_order();
    auto it = std::find(order.begin(), order.end(), seg_name);

    if(it == order.end() || std::next(it) == order.end()) return true;

    std::string next_seg = *std::next(it);

    if(next_seg == "git-branch" && get_git_segment() == "<UNUSED>") return true;
    if(next_seg == "jobs" && get_jobs_segment() == "<UNUSED>") return true;
    if(next_seg == "ssh" && get_ssh_segment() == "<UNUSED>") return true;

    return false;
}


std::string get_jobs_segment(bool only_want_unused_or_not) {
    int uncompleted_jobs = [&](){
        int res = 0;
        for(auto& job : JobCont::jobs) if(job.jobstate != JobCont::State::Completed) ++res;
        return res;
    }();
    if(uncompleted_jobs == 0) return "<UNUSED>";

    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","jobs").value_or(false)) return "";
    if(only_want_unused_or_not) return "<SUCCESS>";

    auto segmentstyle = get_string(j,"segment-style").value_or("rectangle");
    auto first = get_order()[0] == "jobs";
    auto last  = get_order().back() == "jobs";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","jobs").value_or("");
    auto after  = get_string(j,"after","jobs").value_or("");
    auto fg     = get_int_array3(j,"foreground","jobs").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","jobs").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","jobs").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","jobs").value_or("");

    bool next_unused = is_next_segment_unused("jobs");
    return print_segment("jobs", std::to_string(uncompleted_jobs), after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}


std::string get_ssh_segment(bool only_want_unused_or_not) {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","ssh").value_or(false) || !is_ssh_server()) return "<UNUSED>";
    
    if(only_want_unused_or_not) return "<SUCCESS>";

    auto text = get_string(j,"text","ssh").value_or("ssh");

    auto segmentstyle = get_string(j,"segment-style").value_or("rectangle");
    auto first = get_order()[0] == "ssh";
    auto last  = get_order().back() == "ssh";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","ssh").value_or("");
    auto after  = get_string(j,"after","ssh").value_or("");
    auto fg     = get_int_array3(j,"foreground","ssh").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","ssh").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","ssh").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","ssh").value_or("");

    bool next_unused = is_next_segment_unused("ssh");
    return print_segment("ssh", text, after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}

std::string get_git_segment(bool only_want_unused_or_not) {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","git-branch").value_or(false)) return "";
    if(only_want_unused_or_not) return "<SUCCESS>";

    char cwd_buffer[PATH_MAX];
    std::string cwd = getcwd(cwd_buffer,sizeof(cwd_buffer));
    GitRepo repo(cwd);
    std::string branch = repo.get_branch_name();
    if(branch.empty()) return "<UNUSED>";
    else if(only_want_unused_or_not) return "<SUCCESS>";

    auto segmentstyle = get_string(j,"segment-style").value_or("rectangle");
    auto first = get_order().at(0) == "git-branch";
    auto last  = get_order().back() == "git-branch";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","git-branch").value_or("");
    auto after  = get_string(j,"after","git-branch").value_or("");
    auto fg     = get_int_array3(j,"foreground","git-branch").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","git-branch").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","git-branch").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","git-branch").value_or("");

    bool next_unused = is_next_segment_unused("git-branch");
    return print_segment("git-branch", branch, after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}

std::string get_beforeall_segment() {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","before-all").value_or(false)) return "";

    auto characters = get_string(j,"chars","before-all").value_or("");
    return characters;
}

std::string get_time_segment() {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","time").value_or(false)) return "";

    auto showSeconds = get_bool(j, "showSeconds","time").value_or(false);
    auto use24hr = get_bool(j,"twentyfourhr","time").value_or(false);

    time_t t = time(nullptr);
    struct tm* now = localtime(&t);
    char buffer[16];
    if(showSeconds) strftime(buffer,sizeof(buffer),use24hr?"%H:%M:%S":"%I:%M:%S %p",now);
    else strftime(buffer,sizeof(buffer),use24hr?"%H:%M":"%I:%M %p",now);

    auto segmentstyle = get_string(j,"segment-style").value_or("rectangle");
    auto first = get_order()[0] == "time";
    auto last  = get_order().back() == "time";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","time").value_or("");
    auto after  = get_string(j,"after","time").value_or("");
    auto fg     = get_int_array3(j,"foreground","time").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","time").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","time").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","time").value_or("");

    bool next_unused = is_next_segment_unused("time");
    return print_segment("time", buffer, after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}

std::string get_user_segment() {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","user").value_or(false)) return "";

    std::string user = getenv("USER") ? getenv("USER") : "";

    auto segmentstyle = get_string(j,"segment-style").value_or("rectangle");
    auto first = get_order()[0] == "user";
    auto last  = get_order().back() == "user";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","user").value_or("");
    auto after  = get_string(j,"after","user").value_or("");
    auto fg     = get_int_array3(j,"foreground","user").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","user").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","user").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","user").value_or("");

    bool next_unused = is_next_segment_unused("user");
    return print_segment("user", user, after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}

std::string get_group_segment() {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","group").value_or(false)) return "";

    gid_t gid = getgid();
    struct group* grp = getgrgid(gid);
    std::string group = grp ? grp->gr_name : "";

    auto segmentstyle = get_string(j,"segment-style").value_or("rectangle");
    auto first = get_order()[0] == "group";
    auto last  = get_order().back() == "group";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","group").value_or("");
    auto after  = get_string(j,"after","group").value_or("");
    auto fg     = get_int_array3(j,"foreground","group").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","group").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","group").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","group").value_or("");

    bool next_unused = is_next_segment_unused("group");
    return print_segment("group", group, after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}

std::string get_hostname_segment() {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","hostname").value_or(false)) return "";

    char hostname[256];
    gethostname(hostname,sizeof(hostname));

    auto segmentstyle = get_string(j,"segment-style").value_or("rectangle");
    auto first = get_order()[0] == "hostname";
    auto last  = get_order().back() == "hostname";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","hostname").value_or("");
    auto after  = get_string(j,"after","hostname").value_or("");
    auto fg     = get_int_array3(j,"foreground","hostname").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","hostname").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","hostname").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","hostname").value_or("");

    bool next_unused = is_next_segment_unused("hostname");
    return print_segment("hostname", hostname, after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}

std::string get_cwd_segment() {
    auto j = get_json(get_prompt_config_path());
    if(j.empty() || !get_bool(j,"enabled","currentdir").value_or(false)) return "";

    char buffer[512];
    std::string cwd = getcwd(buffer,sizeof(buffer));
    std::string home_str = getenv("HOME");
    if(cwd.starts_with(home_str)) cwd.replace(0,home_str.size(), get_string(j, "homechar", "currentdir").value_or("~"));

    auto segmentstyle = get_string(j,"segment-style").value_or("hardcoded");
    auto first = get_order()[0] == "currentdir";
    auto last  = get_order().back() == "currentdir";
    SegmentStyle sstyle = get_segment_style(segmentstyle, first, last);

    auto before = get_string(j,"before","currentdir").value_or("");
    auto after  = get_string(j,"after","currentdir").value_or("");
    auto fg     = get_int_array3(j,"foreground","currentdir").value_or(std::array<int,3>{256,256,256});
    auto bg     = get_int_array3(j,"background","currentdir").value_or(std::array<int,3>{256,256,256});
    auto bold   = get_bool(j,"bold","currentdir").value_or(false);
    auto nerd_i = get_string(j,"nerd-icon","currentdir").value_or("");

    auto separator = get_string(j, "separator", "currentdir").value_or("/");
    auto shorten = get_bool(j, "shorten", "currentdir").value_or(false);

    io::replace_all(cwd, "/", separator);
    if(shorten) cwd = io::split(cwd, separator).back();

    bool next_unused = is_next_segment_unused("currentdir");
    return print_segment("currentdir", cwd, after, before, bold, nerd_i, fg, bg, sstyle, segmentstyle, next_unused);
}


std::string get_prompt_segment() {
    auto j = get_json(get_prompt_config_path());
    if(j.empty()) return "";
    if(!get_bool(j,"enabled","prompt").value_or(false)) return "";

    auto before = get_string(j,"before","prompt").value_or("");
    auto after = get_string(j,"after","prompt").value_or("");
    auto fg = get_int_array3(j,"foreground","prompt").value_or(std::array<int,3>{256,256,256});
    auto bg = get_int_array3(j,"background","prompt").value_or(std::array<int,3>{256,256,256});
    auto bold = get_bool(j,"bold","prompt").value_or(false);
    auto newlineBefore = get_bool(j,"newlineBefore","prompt").value_or(false);
    auto newlineAfter = get_bool(j,"newlineAfter","prompt").value_or(false);
    auto character = get_string(j,"character","prompt").value_or(">");

    std::string ansi;
    if(fg != std::array<int,3>{256,256,256}) ansi += rgb_to_ansi(fg);
    if(bg != std::array<int,3>{256,256,256}) ansi += rgb_to_ansi(bg,true);
    if(bold) ansi = "\x1b[1m" + ansi;

    std::stringstream out;
    if(newlineBefore) out << "\n" << "\x1b[2K";
      out << before
          << ansi << character << reset
          << after;
    if(newlineAfter) out << "\n";

    return out.str();
}


void draw_prompt() {
    auto j = get_json(get_prompt_config_path());
    if (j.empty()) return;
    std::stringstream prompt;

    for (auto& elm : get_order()) {
        std::string seg_str;
        if(elm == "before-all") seg_str = get_beforeall_segment();
        else if(elm == "time") seg_str = get_time_segment();
        else if(elm == "user") seg_str = get_user_segment();
        else if(elm == "group") seg_str = get_group_segment();
        else if(elm == "hostname") seg_str = get_hostname_segment();
        else if(elm == "currentdir") seg_str = get_cwd_segment();
        else if(elm == "git-branch") seg_str = get_git_segment();
        else if(elm == "jobs") seg_str = get_jobs_segment();
        else if(elm == "ssh") seg_str = get_ssh_segment();

        if(seg_str != "<UNUSED>") prompt << seg_str;
    }


    io::print(prompt.str());

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
      if(char_pos == io::strip_ansi(content).length()) io::print("\x1b[0K"); // Clear the rest of the line from the cursor to visually delete a character if backspace
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

    if(c == 3) { // Ctrl+C, discard input
      io::print(red + "^C" + reset + "\n");
      return "";
    }

    if(c == 4) {
      if(!buffer.empty()) continue;

        if(!JobCont::jobs.empty()) {
            std::vector<JobCont::Job> non_completed_jobs;
            for(auto& job : JobCont::jobs) {
                if(job.jobstate != JobCont::State::Completed) non_completed_jobs.push_back(job);
            }
            if(!non_completed_jobs.empty()) {
                std::string first = JobCont::jobs.size() == 1 ? "There is 1 uncompleted job." : "There are " + std::to_string(JobCont::jobs.size()) + " uncompleted jobs.";
                info::warning(first + " Are you sure you want to exit slash? (Y/N): ");
                char c;
                ssize_t bytesRead = read(STDIN_FILENO, &c, 1);
                if(bytesRead < 0) {
                    info::error("Failed to read stdin: " + std::string(strerror(errno)));
                    return read_input(history_index);
                }
                io::print("\n");
                if(tolower(c) != 'y') {
                    return read_input(history_index);
                }
            }
        }

        io::print(red + "EOF" + reset);
        io::print("\n");
        enable_canonical_mode();
        exit(0);
    }

    if (c == 23) { // Ctrl+W
        if (!buffer.empty()) {
            while (!buffer.empty() && buffer.back() != ' ') {
                buffer.pop_back();
            }
        }

        char_pos = buffer.length();
        redraw_prompt(highl(buffer));
    }

    if(c == 11) {
      buffer = buffer.substr(0, char_pos);
      redraw_prompt(highl(buffer));
      continue;
    }

    if(c == 12) { // Ctrl+L
      io::print("\033[H\033[2J");
      draw_prompt();
      io::print(highl(buffer));
    }

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

