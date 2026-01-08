#include <parser/parser.h>
#include <parser/word.h>
#include <iofuncs.h>

decltype(AST::Word::word) AST::Word::array()
{
  return word;
}

void AST::Word::push(Element element) { word.push_back(element); }

  AST::Word* Parser::word_parser(bool ignore_ws, WordSettings settings) {
    using Modifier = AST::NodeString::Modifier;

    AST::Word* word = new AST::Word();
    while(!at_eof()) {
      if(current().type == TokType::WORD_SEPARATOR || current().type == TokType::STATEMENT_SEPARATOR) {
        if(ignore_ws) { word->push(current().lexeme); advance(); continue; }

        if(current().type == TokType::STATEMENT_SEPARATOR) break;

        advance();
        if(!word->array().empty()) break;

        continue;
      }

      if(current().type == TokType::QUOTE && settings.quotes) {
        std::string quote = current().lexeme;
        AST::NodeString* node = new AST::NodeString();
        node->singular = quote == "'";

        std::string previous_lexeme = previous().lexeme;
        static std::vector<std::pair<std::string, Modifier>> modifiers = {
          {"@", Modifier::Colored},
          {"$", Modifier::ANSI_C},
          {"&", Modifier::Both}
        };

        for(auto& [ch, modifier] : modifiers) {
          if(previous_lexeme.ends_with(ch)) {
            node->modifier = modifier;
            switch(node->modifier) {
              case Modifier::ANSI_C: io::debug("ansi c");
              case Modifier::Colored: io::debug("clored");
              default: {}
            }

            if(!word->array().empty() && word->array().back().is<std::string>()) {
              std::string prev = word->array().back().as<std::string>();
              prev.pop_back();

              word->array().pop_back();
              word->push(prev);
            }
          }
        }

        advance();

        auto tokens = until(TokType::QUOTE, quote);
        advance();
        
        for(auto& t : tokens) node->content.push_back(new AST::Word(t.lexeme));
        word->push(node);
        continue;
      }
      if(current().type == TokType::ESCAPED && settings.escaped) {
        AST::NodeEscaped* node = new AST::NodeEscaped();
        node->what = current().lexeme;
        advance();

        word->push(node);
        continue;
      }

      if(current().check(TokType::PUNCTUATION, "`") && settings.substitution) {
        advance();
        AST::NodeSubstitution* node = new AST::NodeSubstitution();
        node->body = block_parser(until(TokType::PUNCTUATION, "`"));
        advance();

        word->push(node);
        continue;
      }

      if(current().check(TokType::PUNCTUATION, "$(") && settings.substitution) {
        advance();

        AST::NodeSubstitution* node = new AST::NodeSubstitution();
        node->body = block_parser(until(DepthInfo(DepthInfo::Parentheses)));

        word->push(node);
        continue;
      }

      if(current().check(TokType::PUNCTUATION, "$") && settings.variables) {
        advance();
        std::string n = current().lexeme;
        
        std::string name = "";
        for(auto& c : n) {
          if(name.empty() && (c == '!' || c == '@' || c == '#' || c == '$' || c == '*' || c == '_' || c == '?' || c == '-')) {
            name += c;
            break;
          }

          if(!isalnum(c) && c != '_') break;
          name += c;
        }

        AST::NodeVariable* node = new AST::NodeVariable();
        node->name = name;

        word->push(node);
        word->push(n.substr(name.size()));
        advance();

        continue;
      }

      // this point means escaped is disabled
      if(current().type == TokType::ESCAPED) word->push("\\" + current().lexeme);
      else word->push(current().lexeme);

      advance();
    }

    return word;
  }