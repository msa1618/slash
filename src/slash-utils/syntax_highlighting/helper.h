#ifndef SLASH_HELPER_H
#define SLASH_HELPER_H

#include <string>
#include <boost/regex.hpp>

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


#endif //SLASH_HELPER_H
