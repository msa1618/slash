#ifndef SLASH_PARSER_H
#define SLASH_PARSER_H

#include <string>
#include <vector>
#include "lexer.h"
#include "../errors.h"
#include "ast.h"
#include "nodes.h"

  class Parser {
    private:
      struct DepthInfo {
        std::vector<Token> increasers;
        std::vector<Token> decreasers;

        enum pref { Parentheses, Brackets };

        DepthInfo() = default;
        DepthInfo(pref type) {
          if(type == Parentheses) { 
            increasers = {Token(TokType::PUNCTUATION, "("), Token(TokType::PUNCTUATION, "$(")};
            decreasers = {Token(TokType::PUNCTUATION, ")")};
          }
          if(type == Brackets) {
            increasers = {Token(TokType::PUNCTUATION, "{"), Token(TokType::PUNCTUATION, "${")};
            decreasers = {Token(TokType::PUNCTUATION, "}")};
          }
        };
      };

      int i;
      AST::NodeBlock* result;
      std::vector<Token> tokens;
      std::vector<Diagnostic> diagnostics;

      AST::Word* word_parser();
      AST::Node* after_parser(AST::Node* node, bool expects_arguments = false);

      AST::NodeCommand* command_parser();
      AST::NodeExit* exit_parser();

    public:  
      Parser(Lexer lexer) : tokens(lexer.tokens()) { result = new AST::NodeBlock(); }

      // ws = word separator
      Token consume(TokType type, std::string lexeme = "", bool ws_matters = false);
      Token peek(int n = 1, bool ws_matters = false);
      Token& current(bool ws_matters = false);
      Token previous(bool ws_matters = false);
      void advance(bool ws_matters = false);

      decltype(diagnostics) diags();
      bool clean();

      bool at_eof();
      bool handle_eof(); // @return `true`: if an error was thrown

      void error(syntax syn, std::string message);
      void recover();
      void push(AST::Node* node);

      void parse();
      AST::NodeBlock* block();
  };


#endif