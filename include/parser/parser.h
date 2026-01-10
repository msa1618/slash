#ifndef SLASH_PARSER_H
#define SLASH_PARSER_H

#include <string>
#include <vector>
#include "lexer.h"
#include "../errors.h"
#include "ast.h"
#include "nodes.h"
#include <functional>

  struct WordSettings {
    bool quotes, escaped, variables, substitution, brackets;

    WordSettings() : quotes(true), escaped(true), variables(true), substitution(true), brackets(true) {}
  };


  class Parser {
    private:
      struct DepthInfo {
        std::vector<Token> increasers;
        std::vector<Token> decreasers;

        enum pref { Parentheses, Brackets };

        DepthInfo() = default;
        DepthInfo(pref type) {
          if(type == Parentheses) { 
            increasers = {Token(TokType::PUNCTUATION, "("), Token(TokType::PUNCTUATION, "$("), Token(TokType::REDIRECTOR, "<(")};
            decreasers = {Token(TokType::PUNCTUATION, ")")};
          }
          if(type == Brackets) {
            increasers = {Token(TokType::PUNCTUATION, "{"), Token(TokType::PUNCTUATION, "${")};
            decreasers = {Token(TokType::PUNCTUATION, "}")};
          }
        };
      };

      int i{};
      bool backtick_warned{};

      AST::NodeBlock* result;
      std::vector<Token> tokens;
      std::vector<Diagnostic> diagnostics;

      AST::Node* after_parser(AST::Node* node, bool expects_arguments = false);

      // could be command, variable declaration, function decl, ...
      AST::Node* raw_parser();

      AST::NodeCommand* command_parser();
      AST::NodeExit* exit_parser();
      AST::NodeBlock* block_parser(std::vector<Token> tokens);
      AST::NodeIf* if_parser();
      AST::NodeWhile* while_until_parser();
      AST::Node* statement_parser();

      AST::NodeBlock* block_subshell_parser();
      AST::NodeVariableDeclaration* vardec_parser();

      AST::NodeLoop* loop_parser();
      AST::NodeLoopControl* control_parser();

    public:  
      Parser(Lexer lexer) : tokens(lexer.tokens()) { result = new AST::NodeBlock(); }
      Parser(std::vector<Token> tokens) : tokens(tokens) {  result = new AST::NodeBlock(); }

      // ws = word separator
      Token consume(TokType type, std::string lexeme = "", bool ws_matters = false);
      Token peek(int n = 1, bool ws_matters = false);
      Token& current(bool ws_matters = false);
      Token previous(bool ws_matters = false);
      void advance();

      AST::Word* word_parser(bool ignore_ws = false, WordSettings settings = WordSettings());

      decltype(diagnostics) diags();
      bool clean();

      bool at_eof();
      bool handle_eof(); // @return `true`: if an error was thrown

      void error(syntax syn, std::string message);
      void recover();
      void push(AST::Node* node);

      std::vector<Token> until(DepthInfo info);
      std::vector<Token> until(std::function<bool(Token)> function);
      std::vector<Token> until(TokType type, std::string lexeme = "");

      void parse();
      AST::NodeBlock* block();
  };


#endif