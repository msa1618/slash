#ifndef SLASH_LEXER_H
#define SLASH_LEXER_H

#include <string>
#include <vector>

enum class TokType {
  KEYWORD,
  OPERATOR,
  PUNCTUATION,
  REDIRECTOR,
  QUOTE,
  COMMENT,
  RAW,
  INTEGER,
  FLOAT,
  EXECUTION_FLAG,

  WORD_SEPARATOR,
  STATEMENT_SEPARATOR
};
std::string toktype_to_str(TokType type);

struct Token {
  TokType type;
  std::string lexeme;

  Token(TokType type, std::string lexeme) : type(type), lexeme(lexeme) {}
  Token() = default;

  bool check(TokType type, std::string lexeme) {
    return this->type == type && this->lexeme == lexeme;
  }
};

class Lexer {
  private:
    std::string block;
    int i;
    std::vector<Token> result;

  public:
    Lexer(std::string block) : block(block) { begin(); }

    void begin();
    decltype(result) tokens();
    void dump();
};

#endif