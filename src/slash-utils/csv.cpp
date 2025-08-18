#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"


#include <vector>
#include <string>
#include <regex>
#include "../help_helper.h"

class Csv {
  private:
  
std::string strip_ansi(const std::string& str) {
    static const std::regex ansi_regex("\x1B\\[[0-9;]*[mK]");
    return std::regex_replace(str, ansi_regex, "");
}

std::string print_csv_table(std::string csv, std::string separator) {
    csv = io::trim(csv);
    std::vector<std::vector<std::string>> table;
    auto rows = io::split(csv, "\n");
    for (auto& row : rows) {
        table.push_back(io::split(row, separator));
    }

    if (table.empty()) return "";

    int cols = table[0].size();
    std::vector<int> col_widths(cols, 0);

    // Calculate max width per column (using visible length)
    for (auto& row : table) {
        for (int i = 0; i < cols; i++) {
            if (i < row.size()) {
                int len = strip_ansi(row[i]).length();
                if (len > col_widths[i])
                    col_widths[i] = len;
            }
        }
    }

    // Calculate total width including separators " │ " (3 chars each)
    int total_width = 0;
    for (int w : col_widths) total_width += w;
    total_width += 3 * (cols - 1);

    // Print top border
    for (int i = 0; i < total_width; i++) io::print(green + "─" + reset);
    io::print("\n");

    // Print header row
    for (int j = 0; j < cols; j++) {
        std::string cell = j < table[0].size() ? table[0][j] : "";
        int visible_len = strip_ansi(cell).length();
        io::print(green + cell + std::string(col_widths[j] - visible_len, ' ') + reset);
        if (j < cols - 1) io::print(green + " │ " + reset);
    }
    io::print("\n");

    // Print header separator
    for (int i = 0; i < total_width; i++) io::print(green + "─" + reset);
    io::print("\n");

    // Print remaining rows
    for (int i = 1; i < table.size(); i++) {
        for (int j = 0; j < cols; j++) {
            std::string cell = j < table[i].size() ? table[i][j] : "";
            int visible_len = strip_ansi(cell).length();
            io::print(cell + std::string(col_widths[j] - visible_len, ' '));
            if (j < cols - 1) io::print(" │ ");
        }
        io::print("\n");
    }

    // Print bottom border
    for (int i = 0; i < total_width; i++) io::print("─");
    io::print("\n");

    return "";
}


  public:
    Csv() {}

    int exec(std::vector<std::string> args) {
      if (args.empty()) {
        io::print(get_helpmsg({
          "Renders CSV tables in a pretty format",
          {
            "csv <file>",
            "csv [option] <file-or-text>"
          },
          {
            {"-s", "--separator", "Specifies the separator " + yellow + "(NOTE: Use \\| and \\; for these two delimiters)" + reset},
            {"-t", "--text", "The next argument is text (useful for piping)"}
          },
          {
            {"csv table.csv", "Prints table for table.csv data"},
            {"csv -s ; table.csv", "Prints table, knowing the separator is ;"}
          },
          "",
          ""
        }));
        return 0;
      }

      std::vector<std::string> valid_args = {
        "-s", "-t",
        "--separator", "--text"
      };

      bool is_text = false;
std::string separator = ",";
std::string arg;

for (size_t i = 0; i < args.size(); ++i) {
    const std::string& current = args[i];

    // Invalid unknown options
    if (current.starts_with("-") &&
        current != "-s" && current != "--separator" &&
        current != "-t" && current != "--text") {
        info::error("Invalid argument \"" + current + "\"\n");
        return EINVAL;
    }

    // Separator option
    if (current == "-s" || current == "--separator") {
        if (i + 1 >= args.size()) {
            info::error("Expected separator after " + current);
            return EINVAL;
        }
        separator = args[++i]; // use the next argument
        if (separator == "\\|") separator = "|";
        else if (separator == "\\;") separator = ";";
        continue;
    }

    // Text option
    if (current == "-t" || current == "--text") {
        if (i + 1 >= args.size()) {
            info::error("Expected text after " + current);
            return EINVAL;
        }
        arg = args[++i]; // grab the actual text
        is_text = true;
        continue;
    }

    // Positional argument (filename)
    if (!current.starts_with("-") && arg.empty()) {
        arg = current;
    }
}
}
};

  int main(int argc, char* argv[]) {
    Csv csv;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
      args.emplace_back(argv[i]);
    }

    csv.exec(args);
    return 0;
  }