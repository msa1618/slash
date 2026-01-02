#include <parser/parser.h>
#include <parser/word.h>
#include <iofuncs.h>

decltype(AST::Word::word) AST::Word::array()
{
  return word;
}

void AST::Word::push(Element element) { word.push_back(element); }
void AST::Word::push(std::string str) { word.push_back(Element(str)); }

  AST::Word* Parser::word_parser() {
    AST::Word* word = new AST::Word();

    while(current().type != TokType::WORD_SEPARATOR && current().type != TokType::STATEMENT_SEPARATOR && !at_eof()) {
      if(current().check(TokType::PUNCTUATION, "`")) {
        diagnostics.push_back(Diagnostic(Severity::Warning, "backticks are treated as a normal character and not command substitution"));
      }

      if(current().check(TokType::PUNCTUATION, "$")) {
        std::string next = consume(TokType::RAW).lexeme;
        
        std::string name;
        for(auto& c : next) {
          if(!isalnum(c) && c != '_') break;
          name += c;
        }

        AST::NodeVariable* node = new AST::NodeVariable();
        node->name = name;

        word->push(node);
        word->push(next.substr(name.size()));
        advance(true);

        continue;
      }
      word->push(current().lexeme);
      advance(true);
    }

    return word;
  }