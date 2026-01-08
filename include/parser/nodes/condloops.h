#ifndef SLASH_CONDLOOPS_H
#define SLASH_CONDLOOPS_H

#include "../ast.h"

namespace AST {
  struct NodeLoopControl   : Node { bool is_break; std::string n; std::string name() override { return "NodeLoopControl"; } };
  struct NodeLoop          : Node { NodeBlock* body; std::string name() override { return "NodeLoop"; } };
  struct NodeWhile : Node { // while, until
    Node* condition;
    NodeBlock* body;
    bool is_until = false;

    std::string name() override { return "NodeWhile"; }
  };

  struct NodeIf : Node {
    struct Elif { Node* condition; NodeBlock* body; };

    Elif primary;
    std::vector<Elif> elifs;
    NodeBlock* else_block;
  };
  
  struct NodeRForSelect : Node {
    std::vector<Word> list;
    std::string name() override { return "NodeRForSelect"; }
  };
}

#endif