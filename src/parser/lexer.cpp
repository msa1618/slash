#include <parser/lexer.h>
#include <unordered_set>
#include <iofuncs.h>
#include <parser/lexer.h>

std::unordered_set<std::string> keywords() {
  static std::unordered_set<std::string> keywords = {
    "if", "else", "elif", "unless", "while", "until",
    "loop", "when", "break", "continue",
    "function", "return", "class", "inherits",
    "public", "private", "operators", "select",
    "match", "try", "catch", "finally", "throw",
    "const", "exit", "logout", "end", "export",
    "module", "import", "exit", "logout", "in",
    "exec", "time", "then", "fi", "do", "done", "cd",
    "source", ".", "export", "declare", "readonly",
    "and", "is", "or", "not", "true", "false", "null"
  };

  return keywords;
}

std::unordered_set<std::string> punctuation() {
    static std::unordered_set<std::string> punctuators = {
      "{", "}",   "$(",
      "(", ")",   "`",
      "[", "]",
      ":", ",",

      "$", "${"
    };
    return punctuators;
}

std::unordered_set<std::string> operators() {
    static std::unordered_set<std::string> operators = {
      "+", "-", "*", "/",
      "&", "|", "^", "**",
      "%", "=",

      "<=", ">=",

      "!=", "==", "||", "&&",
      "~=",

      "+=", "-=", "/=", "*=",

      "&=", "|=", "^=",
      "+=", "-=", "*=", "/=",
      "**=", "%=", "++", "--"
    };
    return operators;
}
std::unordered_set<std::string> redirectors() {
  static std::unordered_set<std::string> redirectors = {
    ">",  ">>",
    "<", "<<", "<<<",
    "<(",
    ">|"
  };
  return redirectors;
}
std::unordered_set<std::string> execflags() {
  static std::unordered_set<std::string> execflags = {
    "@e", "@s", "@v", "@r", "@O", "@E", "@t"
  };
  return execflags;
}
bool is_integer(std::string str) {
  if(str.empty()) return false;

  if(str.starts_with("0x") || str.starts_with("0X")) {
    for(auto& c : str.substr(2)) {
      if(!isxdigit(c)) return false;
    }
    return true;
  }
  if(str.starts_with("0b") || str.starts_with("0B")) {
    for(auto& c : str.substr(2)) {
      if(c != '0' && c != '1') return false;
    }
    return true;
  }
  if(str.starts_with("0")) {
    for(auto& c : str.substr(1)) {
      if(c < '0' || c > '7') return false;
    }
    return true;
  }

  for(auto& c : str) {
    if(!isdigit(c)) return false;
  }
  return true;
}
bool is_float(std::string str) {
  if(str.empty()) return false;

  bool dot_used = false;
  for(int i = 0; i < str.size(); i++) {
    char c = str[i], next = i + 1 < str.size() ? str[i + 1] : 0;

    if(c == '.') {
      if(dot_used) return false;
      dot_used = true;

      if(i == 0) return false;
      if(i == str.size() - 1) return false;
    }
    if(!isdigit(c)) return false;
  }
  return true;
}

void Lexer::begin() {
  std::string cw;

  auto handle_word = [&](){
    if(cw.empty()) return;

    if(keywords().find(cw) != keywords().end()) result.push_back(Token(TokType::KEYWORD, cw));
    else if(execflags().find(cw) != execflags().end()) result.push_back(Token(TokType::EXECUTION_FLAG, cw));
    else if(is_integer(cw))                     result.push_back(Token(TokType::INTEGER, cw));
    else if(is_float(cw))                       result.push_back(Token(TokType::FLOAT, cw));
    else                                        result.push_back(Token(TokType::RAW, cw));

    cw.clear();
  };
  auto check_and_handle_opers = [&](int& i){
    char c = block[i];
    char next = i + 1 < block.size() ? block[i + 1] : 0;
    char next2 = i + 2 < block.size() ? block[i + 2] : 0;

    std::vector<std::string> trio;

    if (next2) trio.push_back(std::string({c, next, next2}));
    if (next) trio.push_back(std::string({c, next}));
    trio.push_back(std::string(1, c));


    for(auto& s : trio) {
      if(redirectors().find(s) != redirectors().end()) {
        handle_word();
        result.push_back(Token(TokType::REDIRECTOR, s));
        cw.clear();
        i += s.size() - 1;
        return true;
      }
      if(operators().find(s) != operators().end()) {
        handle_word();
        result.push_back(Token(TokType::OPERATOR, s));

        cw.clear();
        i += s.size() - 1;
        return true;
      }
      if(punctuation().find(s) != punctuation().end()) {
        handle_word();
        result.push_back(Token(TokType::PUNCTUATION, s));

        cw.clear();
        i += s.size() - 1;
        return true;
      }
    }

    return false;
  };

  for(int i = 0; i < block.size(); i++) {
    char c = block[i];

    if(c == '#') {
      while(i < block.size() && block[i] != '\n') {
        i++;
      }
      continue;
    }

    if(isspace(c)) {
      handle_word();

      result.push_back(Token(TokType::WORD_SEPARATOR, ""));

      bool started = false;
      while(i + 1 < block.size() && isspace(block[i + 1]) && block[i + 1] != '\n' && block[i + 1] != ';') {
        started = true;
        i++;
      }

      continue;
    }

    if(c == ';' || c == '\n') {
      result.push_back(Token(TokType::STATEMENT_SEPARATOR, {c}));
      continue;
    }
    
    bool matched = check_and_handle_opers(i);
    if (matched) continue; // skip adding to cw

    cw += c;
  }
  if(!cw.empty()) handle_word();
}

decltype(Lexer::result) Lexer::tokens() {
  return result;
}

void Lexer::dump() {
  for(auto& t : tokens()) {
      io::println(cl::yellow + toktype_to_str(t.type) + cl::reset +
                  "(" + cl::green + t.lexeme + cl::reset + ")");
  }
}
std::string toktype_to_str(TokType type) {
  switch (type) {
  case TokType::KEYWORD:
    return "keyword";
  case TokType::OPERATOR:
    return "operator";
  case TokType::PUNCTUATION:
    return "punctuation";
  case TokType::REDIRECTOR:
    return "redirector";
  case TokType::QUOTE:
    return "quote";
  case TokType::COMMENT:
    return "comment";
  case TokType::RAW:
    return "raw";
  case TokType::INTEGER:
    return "integer";
  case TokType::FLOAT:
    return "float";
  case TokType::EXECUTION_FLAG:
    return "execution_flag";
  case TokType::WORD_SEPARATOR:
    return "word_separator";
  case TokType::STATEMENT_SEPARATOR:
    return "statement_separator";
  default:
    return "unknown";
  }
}
