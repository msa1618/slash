#include "../ast.h"

namespace AST {
  class Word;

  struct NodeCommand : Node {
    Word* program;
  };
  struct NodeConnector : Node {
    std::string connector;

    Node* left;
    Node* right;
  };

  struct NodeExit : Node {
    std::string code;
  };
}