#include "../ast.h"

namespace AST {
  struct NodeVariableDeclaration : Node {
    std::string name;
  };
}