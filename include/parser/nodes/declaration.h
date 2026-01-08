#include "../ast.h"

namespace AST {
  struct NodeVariableDeclaration : Node {
    std::string variable;
    

    std::string name() override { return "NodeVariableDeclaration"; }
  };
}