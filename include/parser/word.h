#ifndef SLASH_WORD_H
#define SLASH_WORD_H

#include <string>
#include <vector>
#include <variant>
#include "ast.h"

namespace AST {
  class Word;
  struct Node;
  
  struct NodeVariable : Node {
    std::string name;
  };
  struct NodeSubstitution : Node {
    NodeBlock* body;
  };
  struct NodeEscaped : Node { std::string what; };
  struct NodeString : Node {
    enum class Modifier {
      None,
      ANSI_C,
      Colored,
      Both
    };

    Modifier modifier = Modifier::None;
    std::vector<Word*> content;
    bool singular;
  };
 
  class Word {
    private:
      class Element {
        private:
          std::variant<std::string, NodeVariable*, NodeSubstitution*, NodeString*, NodeEscaped*> value;

        public:
          Element(std::string str) : value(str) {}
          Element(NodeVariable* node) : value(node) {}
          Element(NodeSubstitution* node) : value(node) {}
          Element(NodeString* node) : value(node) {};
          Element(NodeEscaped* node) : value(node) {};
 
          template <typename T>
          bool is() { return std::holds_alternative<T>(value); }

          template <typename T>
          T as() { return std::get<T>(value); }
      };

      std::vector<Element> word{};

    public:
      Word(std::string str) { word.push_back(str); }
      Word() = default;

      decltype(word) array();
      void push(Element element);
  };
}

#endif