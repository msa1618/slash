#include "../ast.h"

namespace AST {
  struct Type {
    std::string name;

    Type(std::string name) : name(name) {}
    Type() : name("") {}
    bool specified() { return name.empty(); }
  };

  struct NodeVariableDeclaration : Node {
    std::string variable;
    Type type;
    Word* value;
    bool is_const; // uses_sh_style means readonly instead of const is used

    std::string name() override { return "NodeVariableDeclaration"; }
  };
}