#ifndef SLASH_WORD_H
#define SLASH_WORD_H

#include <string>
#include <vector>
#include <variant>
#include "ast.h"

namespace AST {
  struct Node;

  struct NodeVariable : Node {
    std::string name;
  };

  class Word {
    private:
      class Element {
        private:
          std::variant<std::string, NodeVariable*> value;

        public:
          Element(std::string str) : value(str) {}
          Element(NodeVariable* node) : value(node) {}

          template <typename T>
          bool is() { return std::holds_alternative<T>(value); }

          template <typename T>
          T as() { return std::get<T>(value); }
      };

      std::vector<Element> word;

    public:
      decltype(word) array();
      void push(Element element);
      void push(std::string str);
  };
}

#endif