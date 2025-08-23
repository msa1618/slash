#include "cmd_highlighter.h"
#include <boost/regex.hpp>
#include "abstractions/definitions.h"
#include "abstractions/json.h"

std::string highlight(const std::string& input,
                      const std::vector<std::pair<boost::regex, std::string>>& patterns) {
  std::string result;
  size_t pos = 0;
  size_t len = input.size();

  // We'll find all matches of all patterns and sort them by position to colorize properly
  struct Match {
    size_t start;
    size_t end;
    std::string color;
  };

  std::vector<Match> matches;

  for (auto& [pattern, color] : patterns) {
    boost::sregex_iterator it(input.begin(), input.end(), pattern);
    boost::sregex_iterator end;
    while (it != end) {
      auto m = *it;
      matches.push_back({(size_t)m.position(), (size_t)m.position() + m.length(), color});
      ++it;
    }
  }

  // Sort matches by start position
  std::sort(matches.begin(), matches.end(),
            [](const Match& a, const Match& b) { return a.start < b.start; });

  size_t last_end = 0;
  for (const auto& m : matches) {
    // Skip overlaps
    if (m.start < last_end) continue;

    // Append text before match
    if (m.start > last_end)
      result.append(input.substr(last_end, m.start - last_end));

    // Append colored match
    result.append(m.color + input.substr(m.start, m.end - m.start) + "\033[0m");
    last_end = m.end;
  }

  // Append any remaining text
  if (last_end < len)
    result.append(input.substr(last_end));

  return result;
}

std::string rgb_to_ansi_2(std::array<int, 3> rgb, bool bg = false) {
  int r = rgb[0], g = rgb[1], b = rgb[2];
  if(r == 256 || g == 256 || b == 256) return "";
  std::stringstream res;
  if(bg) res << "\x1b[48;2;" << r << ";" << g << ";" << b << "m";
  else res << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
  return res.str();
}

std::string get_ansi(nlohmann::json& j, std::string elm) {
  if(!j.contains(elm)) return "";

  std::stringstream ss;
  if(get_bool(j, "bold", elm).value_or(false)) ss << bold;
  if(get_bool(j, "italic", elm).value_or(false)) ss << italic;
  if(get_bool(j, "underline", elm).value_or(false)) ss << underline;

  ss << rgb_to_ansi_2(get_int_array3(j, "foreground", elm).value_or(std::array<int, 3>({256, 256, 256})));
  ss << rgb_to_ansi_2(get_int_array3(j, "background", elm).value_or(std::array<int, 3>({256, 256, 256})), true);

  return ss.str();
}

std::string get_syntax_highlighting_theme_path() {
  std::string home = getenv("HOME");
  auto j = get_json(home + "/.slash/config/settings.json");
  if(j.empty()) return home + "/.slash/config/syntax-highlighting-themes/default.json";

  return get_string(j, "pathOfSyntaxHighlightingTheme").value_or(home + "/.slash/config/syntax-highlighting-themes/default.json");
}

std::string highl(std::string prompt) {
  auto j = get_json(get_syntax_highlighting_theme_path());

  boost::regex quotes(R"((["'])(?:\\.|(?!\1).)*\1)");
  boost::regex comments("\\#.*");
  boost::regex operators(">>|&&|\\|\\||[|&>]");
  boost::regex paths("(/[^\\s|&><#\"]+)|(\\.{1,2}(/[^\\s|&><#\"]*)*)|(~)");
  boost::regex flags("-[A-Za-z0-9\\-_]+");
  boost::regex numbers("[0-9]");
  boost::regex quote_pref("(E|@)(?=\"[^\"]*\")");
  // I know its lengthy but boost::regex doesnt support variable length lookbehinds. Atleast theres lookbehind support unlike std::regex
  boost::regex cmds(R"((?:^|\s*(?<=&&)|\s*(?<=\|)|\s*(?<=;))\s*([^\s]+))"); 
  boost::regex exec_flags(R"(@(r|t|o|O|e))");
  boost::regex links(R"([a-zA-Z][a-zA-Z0-9+.-]*:\/\/[^\s/$.?#].[^\s]*)");
  boost::regex subcommands(R"(\b[A-Za-z0-9\-_]+\b)");

  const std::string cmd_color         = get_ansi(j, "command");
  const std::string number_color      = get_ansi(j, "numbers");
  const std::string flag_color        = get_ansi(j, "flags");
  const std::string path_color        = get_ansi(j, "paths");
  const std::string comment_color     = get_ansi(j, "comments");
  const std::string quote_color       = get_ansi(j, "quotes");
  const std::string quote_pref_color  = get_ansi(j, "quotes_pref");
  const std::string link_color        = get_ansi(j, "links");
  const std::string subcommand_color  = get_ansi(j, "subcommand");
  const std::string exec_flags_color  = get_ansi(j, "exec_flags");
  const std::string operator_color    = get_ansi(j, "operators");

  std::vector<std::pair<boost::regex, std::string>> patterns = {
      {exec_flags, exec_flags_color},
      {numbers, number_color},
      {flags, flag_color},
      {paths, path_color},
      {comments, comment_color},
      {cmds, cmd_color},
      {operators, operator_color},
      {quote_pref, quote_pref_color},
      {links, link_color},
      {quotes, quote_color},
      {subcommands, subcommand_color},
      {operators, operator_color}
  };

  return highlight(prompt, patterns);
}