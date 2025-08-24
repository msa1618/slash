#include "../abstractions/iofuncs.h"
#include "../abstractions/info.h"

#include <bitset>
#include <sstream>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <iomanip>
#include <regex>

#include "../help_helper.h"

// Global CRC32 table
uint32_t crc32_table[256];

void init_crc32_table() {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        crc32_table[i] = crc;
    }
}

class Sumcheck {
  private:
    std::string strip_ansi(const std::string& input) {
        std::regex ansi_pattern("\x1b\\[[0-9;]*[A-Za-z]");
        return std::regex_replace(input, ansi_pattern, "");
    }

    uint32_t crc32(std::string data) {
        init_crc32_table();
        uint32_t crc = 0xFFFFFFFF;
        for (unsigned char byte : data) {
            crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
        }
        return ~crc;
    }

    int get_vrc(std::string data) {
      int ones = 0;
      for(auto& byte : data) {
        ones += std::bitset<8>(byte).count();
      }
      return ones % 2 == 0 ? 0 : 1;
    }

    uint8_t get_lrc(std::string data) {
      uint8_t res = 0;
      for(auto& byte : data) res ^= byte;
      return res;
    }

    std::string to_hex(unsigned char* data, int len) {
      std::stringstream ss;
      for(int i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
      }
      return ss.str();
    }

    std::string get_md5(std::string input) {
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), digest);
        return to_hex(digest, MD5_DIGEST_LENGTH);
    }

    std::string get_sha256(std::string input) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), digest);
        return to_hex(digest, SHA256_DIGEST_LENGTH);
    }

    int sumcheck(std::string content, bool is_file, bool no_color, bool vrc, bool lrc, bool crc, bool md5, bool sha256) {
      std::string content_to_use;
      if(!is_file) content_to_use = content;
      else {
        auto cnt = io::read_file(content);
        if(!std::holds_alternative<std::string>(cnt)) {
          std::string error = std::string("Failed to read file: ") + strerror(errno);
          info::error(error, errno);
          return -1;
        }
        content_to_use = std::get<std::string>(cnt);
      }

      std::vector<std::pair<std::string, std::string>> results;

      std::string ansi_green = no_color ? "" : "\x1b[38;2;56;102;65m";
      std::string ansi_orange = no_color ? "" : "\x1b[38;2;244;162;97m";

      if(vrc) {
        int v = get_vrc(content_to_use);
        results.push_back({"VRC", v == 0 ? "0" : (ansi_green + "1" + reset)});
      }

      if(lrc) {
        std::bitset<8> bin(get_lrc(content_to_use));
        std::string str = bin.to_string();
        io::replace_all(str, "1", ansi_green + "1" + reset);
        results.push_back({"LRC", str});
      }

      if(crc) {
        std::bitset<32> bin(crc32(content_to_use));
        std::string str = bin.to_string();
        io::replace_all(str, "1", ansi_green + "1" + reset);
        results.push_back({"CRC32", str});
      }

      if(md5) {
        results.push_back({"MD5", ansi_orange + get_md5(content_to_use) + reset});
      }

      if(sha256) {
        results.push_back({"SHA-256", ansi_orange + get_sha256(content_to_use) + reset});
      }

      if(results.size() == 1) {
        io::print(results[0].second + "\n");
        return 0;
      }

      int longest_res_length = 0;
      for(auto& [alg, result] : results) {
        int stripped_res_len = strip_ansi(result).length();
        if(stripped_res_len > longest_res_length) {
          longest_res_length = stripped_res_len;
        }
      }

      io::print("  Alg  │  Result\n");
      io::print("───────┼");

      for(int i = 0; i < longest_res_length; i++) {
        io::print("─");
      } io::print("\n");

      for(auto& [alg, result] : results) {
        alg.resize(7, ' ');
        io::print(alg);
        io::print("│");
        io::print(result);
        io::print("\n");
      }
      return 0;
    }

  public:
    Sumcheck() {}

    int exec(std::vector<std::string> args) {
      if(args.empty()) {
        io::print(get_helpmsg({
          "Computes various checksums",
          {
            "sumcheck [options] <file-or-text>"
          },
          {
            {"-v", "--vrc", "Computes Vertical Redundancy Check (VRC)"},
            {"-l", "--lrc", "Computes Longitudinal Redundancy Check (LRC)"},
            {"-c", "--crc32", "Computes Cyclic Redundancy Check (CRC32)"},
            {"-m", "--md5", "Computes MD5"},
            {"-s", "--sha256", "Computes SHA-256"},
            {"-t", "--text", "The incoming argument is text and not a path"},
            {"-n", "--no-colo[u]r", "Outputs without color"}
          },
          {
            {"sumcheck -c packet.txt", "Computes CRC32 of packet.txt"},
            {"sumcheck -s -t \"sup gng\"", "Computes SHA-256 of the argument"},
            {"echo \"Example content\" | sumcheck -m -t", "Computes MD5 of the piped content"}
          },
          "",
          ""
        }));
          return 0;
      }

      std::vector<std::string> valid_args = {
        "-v", "--vrc",
        "-l", "--lrc",
        "-c", "--crc32",
        "-s", "--sha256",
        "-m", "--md5",
        "-t", "--text",
        "-n", "--no-color", "--no-colour"
      };

      bool vrc = false;
      bool lrc = false;
      bool crc = false;
      bool sha = false;
      bool md5 = false;

      bool isfile  = true;
      bool nocolor = isatty(STDOUT_FILENO);
      std::string a;

      for(auto& arg : args) {
        if(!io::vecContains(valid_args, arg) && arg.starts_with("-")) {
          info::error("Invalid argument \"" + arg + "\"");
          return -1;
        }

        if (arg == "-v" || arg == "--vrc") vrc = true;
        else if (arg == "-l" || arg == "--lrc") lrc = true;
        else if (arg == "-c" || arg == "--crc32") crc = true;
        else if (arg == "-s" || arg == "--sha256") sha = true;
        else if (arg == "-m" || arg == "--md5") md5 = true;
        else if (arg == "-t" || arg == "--text") isfile = false;
        else if(arg == "-n" || arg == "--no-color" || arg == "--no-colour") nocolor = true;

        if(!arg.starts_with("-")) a = arg;
      }

      if(a.empty()) {
        if(isfile) {
          info::error("No file specified!");
          return -1;
        } else {
          std::stringstream ss;
          ss << std::cin.rdbuf();
          a = ss.str();
        }
      }

      return sumcheck(a, isfile, nocolor, vrc, lrc, crc, md5, sha);
    }
};

int main(int argc, char* argv[]) {
    Sumcheck sumcheck;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    return sumcheck.exec(args);
}
