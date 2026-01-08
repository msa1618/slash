#include "../ast.h"

namespace AST {
  class Word;

  struct NodeCommand : Node {
    Word* program;
    std::string name() override { return "NodeCommand"; }
  };
  struct NodeConnector : Node {
    std::string connector;

    Node* left;
    Node* right;

    std::string name() override { return "NodeConnector"; }
  };

  struct NodeExit : Node {
    std::string code;
    std::string name() override { return "NodeExit"; }
  };
}