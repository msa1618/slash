#include "help.h"
#include "../cmd_highlighter.h"

std::string osc_lnk(const std::string& url, const std::string& text) {
    return "\033]8;;" + url + "\033\\" + text + "\033]8;;\033\\";
}

int help() {
  std::stringstream ss;

  ss << yellow << "slash " << reset << " version 1.0\n\n";
  ss << "Welcome to slash, a lightweight, open-source, and feature-rich shell.\n\n";
  
  ss << green << "\"Where is everything stored?\"\n" << reset;
  ss << red << "  • History: " << reset << "  ~/.slash/.slash_history\n";
  ss << red << "  • Aliases: " << reset << "  ~/.slash/.slash_aliases\n";
  ss << red << "  • Variables: " << reset << "~/.slash/.slash_variables\n";
  ss << red << "  • Startup: " << reset << "~/.slash/.slash_startup_commands\n\n";

  ss << red << "  • Prompt: " << reset << "~/.slash/config/prompts/" << gray << "<selected_prompt>\n";
  ss << red << "  • Settings: " << reset << "~/.slash/config/settings.json\n\n";

  ss << green << "Built-in commands\n" << reset;
  ss << yellow << "  • cd:    " << reset << "Change current directory\n";
  ss << yellow << "  • alias: " << reset << "Manipulate aliases\n";
  ss << yellow << "  • var:   " << reset << "Manipulate variables\n";
  ss << yellow << "  • clear: " << reset << "Clear terminal\n";
  ss << yellow << "  • jobs:  " << reset << "View jobs\n\n";

  ss << yellow << "  • slash-greeting: " << reset << "Display the greeting\n\n";

  ss << green <<  "Combinations\n" << reset;
  ss << blue << "  • Ctrl+C:" << reset << " Send SIGINT to the currently running process\n";
  ss << "            Depending on how the program handles it, this may either:\n";
  ss << "              ◦ Terminate it\n";
  ss << "              ◦ Display a message showing a better way to quit it\n";
  ss << "              ◦ Simply get ignored\n"; 
  ss << blue << "  • Ctrl+Z:" << reset << " Stop a process temporarily. It can be resumed with " << highl("jobs -r <pid>\n") << reset;
  ss << "            Stopped processes can be viewed with " << highl("jobs\n\n") << reset;

  ss << green << "Special characters\n" << reset;
  ss << cyan << "  • &:" << reset << "Start process in the background\n\n";
  ss << "   slash offers a list of special flags, not found in other shells,\n called \"execution flags\"\n\n";
  ss << magenta << "    ◦ @t:" << reset << " Start a process with a timer\n";
  ss << magenta << "    ◦ @r:" << reset << " Repeat command until success. Useful in networking\n";
  ss << magenta << "    ◦ @o:" << reset << " Only print stdout\n";
  ss << magenta << "    ◦ @O:" << reset << " Only print stderr\n";
  ss << magenta << "    ◦ @e:" << reset << " Print exit code at exit\n\n";

  ss << green << "Special quotes\n" << reset;
  ss << "  slash offers two special useful quotes\n";
  ss << "    • " << yellow << "E" << reset << green << "\"\"" << reset << ": All escape sequences and characters inside are escaped.\n";
  ss << "    • " << yellow << "@" << reset << green << "\"\"" << reset << ": Inside, you can add brackets with standard ANSI colors inside,\n";
  ss << "           like (green), in addition to (gray) and (orange). You can also add\n";
  ss << "           formatting options, like (bold), (blink), (italic), (overline)..\n";

  ss << green << "Do you:\n" << reset;
  ss << "  1. Want to report a bug: Open an issue at slash's GitHub repository: " << blue << osc_lnk("https://github.com/msa1618/slash/issues", blue + "msa1618/slash" + reset) << "\n";
  ss << "  2. Donate or sponsor: I love you for even thinking about that. Also use the GitHub repository\n";
  ss << "  3. Want slash-utils help instead: Use " << highl("help --slash-utils") << "\n";  

  io::print(ss.str());
  return 0;
}

int slash_utils_help() {
  std::stringstream ss;
  ss << yellow << "slash-utils " << reset << "v0.9.0\n";
  ss << "slash-utils is a suite of utilities, with vibrant output and many utilities\n\n";
  
  ss << green << "All slash-utils\n" << reset << "";

  std::vector<std::pair<std::string, std::string>> commands = {
    {"acart", "Print ASCII art using a FIGlet font file."},
    {"ansi", "Display all ANSI colors and all manipulation codes"},
    {"ascii", "Display all ASCII characters, and the extended characters"},
    {"cmsh", "A calculator mini-shell which supports functions and constants like PI"},
    {"create", "Create files, soft and hard links, and named pipes (FIFOs)"},
    {"csv", "Print CSV data in a pretty format"},
    {"datetime", "Display the current locale's datetime, and also print formatted datetimes"},
    {"del", "Delete or trash files and directories"},
    {"disku", "Show how much disk space each file and directory in the current working directory use"},
    {"dump", "Dump files or text in hexadecimal, octal, decimal, or binary"},
    {"echo", "Print arguments to standard output, with many flags like --typewriter and --rainbow"},
    {"encode", "Encode files or text in Base64, Base32, and ROI-13"},
    {"eol", "Display and change file end-of-lines (EOLs)"},
    {"fnd", "Find files in a specified directory by their name, extension, type, inode ID, etc"},
    {"listen", "Listen for changes in a file, like creation, modification, and deletion"},
    {"ls", "List all files and directories in a specified directory. Has Git support and trees"},
    {"lynx", "Read files with syntax highlighting, Git support, and many options, like printing in reverse or sorting."},
    {"md", "Render Markdown files in a pretty format"},
    {"mkdir", "Create new directory"},
    {"move", "Move a file or directory from one place to another, with cross-filesystem and undoing support"},
    {"netinfo", "Shows information about the current network, like your current IP and MAC address, default gateway..."},
    {"pager", "Used for pagination"},
    {"perms", "View and change permissions"},
    {"ren", "Rename files and directories"},
    {"srch", "A TUI finder for environmental variables, history, and files. It's like a file explorer for files"},
    {"sumcheck", "Print VRC, LRC, CRC32, MD5, or SHA-256 of files or text"},
    {"textmt", "Get various metadata about files or text, like amount of characters, most used character.."}
  };

  int longest_name_length = 0;
  for(auto& [name, desc] : commands) {
    if(name.length() > longest_name_length) longest_name_length = name.length();
  }
  longest_name_length += 3; // Padding

  for(auto& [name, desc] : commands) {
    name.resize(longest_name_length, ' ');
    ss << yellow << name << reset << desc << "\n";
  }
  io::print(ss.str());

  return 0;
}


