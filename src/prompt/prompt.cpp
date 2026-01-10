#include <prompt/prompt.h>
#include <parser/lexer.h>
#include <parser/parser.h>
#include <interpreter/interpreter.h>
#include <global.h>
#include <sstream>
#include <iofuncs.h>
#include <limits.h>
#include <grp.h>
#include <pwd.h>

std::string Prompt::parse_ps(std::string value) {
  Lexer lexer(value);
  Parser parser(lexer.tokens());

  WordSettings settings;
  settings.escaped = false;
  
  //std::string str = Global::interpreter.expand(parser.word_parser(true, settings));
  std::string result;

  time_t now = time(nullptr);
  struct tm *tmp(localtime(&now));

  for(int i = 0; i < value.length(); i++) {
    char c = value[i];
    char next = i + 1 < value.size() ? value[i + 1] : 0;

    if(c == '[') {
      i++;

      std::string between;
      while(i < value.size()) {
        if(value[i] == ']') { break; }

        between += value[i];
        i++;
      }

      static std::unordered_map<std::string, std::string> mappings = {
        {"black",   "\x1b[30m"},
        {"red",     "\x1b[31m"},
        {"green",   "\x1b[32m"},
        {"yellow",  "\x1b[33m"},
        {"blue",    "\x1b[34m"},
        {"magenta", "\x1b[35m"},
        {"cyan",    "\x1b[36m"},
        {"white",   "\x1b[37m"},
        {"reset",   "\x1b[0m"},

        {"bold", "\x1b[1m"}
      };

      if(mappings.find(between) != mappings.end()) result += mappings[between];
      else result += "[" + between + "]";
    }

    else if(c == '\\' && next) {
      switch(next) {
        case 'w': {
          char buffer[PATH_MAX];
          if(!getcwd(buffer, PATH_MAX - 1)) break;

          std::string cwd = buffer;
          if(getenv("HOME")) {
            std::string home = getenv("HOME");
            if(cwd.starts_with(home)) {
              cwd = "~" + cwd.substr(home.length()); 
            }
          }

          result += cwd;
          break;
        }
        case '$': {
          if(geteuid() == 0) result += "#";
          else result += "$";

          break;
        }
        case 'u': {
          passwd* pw = getpwuid(getuid());
          if(pw) result += pw->pw_name;
          break;
        }
        case 'g': {
          group* grp = getgrgid(getgid());
          if(grp) result += grp->gr_name;
          break;
        }
        case 'h': {
          char hostname[1024];

          if(gethostname(hostname, sizeof(hostname) - 1) == 0) result += hostname;
          break;
        }

        case 'a':  { result += "\a"; break; }
        case 'n':  { result += "\n"; break; }
        case '\\': { result += "\\"; break; }
        case 'r':  { result += "\r"; break; }
        case 'e':  { result += "\x1b"; break; }

        // no meaning in display
        case '[': case ']': {
          i++;
          continue;
        }

        case 'd': {
            char buffer[128];
            strftime(buffer, sizeof(buffer) - 1, "%a %b %d", tmp);
            result += buffer;
            break;
        }
        case 't': {
            char buffer[128];
            strftime(buffer, sizeof(buffer) - 1, "%H:%M:%S", tmp);
            result += buffer;
            break;
        }
        case 'T': {
            char buffer[128];
            strftime(buffer, sizeof(buffer) - 1, "%I:%M:%S", tmp);
            result += buffer;
            break;
        }
        case '@': {
            char buffer[128];
            strftime(buffer, sizeof(buffer) - 1, "%I:%M %p", tmp);
            result += buffer;
            break;
        }
        case 'A': case 'R': {
            char buffer[128];
            strftime(buffer, sizeof(buffer) - 1, "%H:%M", tmp);
            result += buffer;
            break;
        }
        case 'D': {
          std::string format;

          i++;
          if(value[i] == '{') {
            while(i < value.size()) {
              if(value[i] == '}') { break; }

              format += value[i];
              i++;
            }

            char buffer[1024];
            strftime(buffer, sizeof(buffer) - 1, format.c_str(), tmp);
            result += buffer;

            break;
          }
        }

        default: {
          result += c;
          if(next) result += next;
          break;
        }
      }
      i++;
      continue;
    }
    else result += c;
  }

  return result;
} 