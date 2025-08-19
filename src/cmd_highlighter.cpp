#include "cmd_highlighter.h"
#include <boost/regex.hpp>
#include "abstractions/definitions.h"

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

std::string highl(std::string prompt) {
  // Right now it's hardcoded. No matter how hard I tried, I couldn't get JSON to work with it.
  // It's better than no highlighting tho :)
  boost::regex quotes(R"((["'])(?:\\.|(?!\1).)*\1)");
  boost::regex comments("\\#.*");
  boost::regex operators(">>|&&|\\|\\||[|&>]");
  boost::regex paths("(/[^\\s|&><#\"]+)|(\\.{1,2}(/[^\\s|&><#\"]*)*)|(~)");
  boost::regex flags("-[A-Za-z0-9\\-_]+");
  boost::regex numbers("[0-9]");
  boost::regex quote_pref("(E|@)(?=\"[^\"]*\")");
  // I know its lengthy but boost::regex doesnt support variable length lookbehinds. Atleast theres lookbehind support unlike std::regex
  boost::regex cmds(R"((?:^|\s*(?<=&&)|\s*(?<=\|)|\s*(?<=;))\s*([^\s]+))"); 
  boost::regex opers(R"((&&|\|\||\||;))");
  boost::regex exec_flags(R"(@(r|t|o|O|e))");
  boost::regex links(R"([a-zA-Z][a-zA-Z0-9+.-]*:\/\/[^\s/$.?#].[^\s]*)");
  boost::regex subcommands(R"(\b[A-Za-z0-9\-_]+\b)");

  const std::string cmd_color         = "\x1b[38;2;96;213;200m";
  const std::string number_color      = "\x1b[38;2;52;160;164m";
  const std::string flag_color        = "\x1b[38;2;131;197;190m";
  const std::string path_color        = "\x1b[38;2;221;161;94m";
  const std::string comment_color     = "\x1b[38;2;73;80;87m";
  const std::string quote_color       = "\x1b[38;2;88;129;87m";
  const std::string quote_pref_color  = "\x1b[38;2;221;161;94m";
  const std::string link_color        = underline + "\x1b[38;2;17;138;178m";
  const std::string subcommand_color  = "\x1b[38;2;226;149;120m";
  const std::string exec_flags_color  = magenta;

  std::vector<std::pair<boost::regex, std::string>> patterns = {
      {exec_flags, exec_flags_color},
      {numbers, number_color},
      {flags, flag_color},
      {paths, path_color},
      {comments, comment_color},
      {cmds, cmd_color},
      {opers, reset},
      {quote_pref, quote_pref_color},
      {links, link_color},
      {quotes, quote_color},
      {subcommands, subcommand_color}
  };

  return highlight(prompt, patterns);
}