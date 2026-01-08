#include <interpreter/interpreter.h>
#include <parser/nodes.h>

Interpreter::Status Interpreter::i_loop(AST::NodeLoop* node) {
  while(true) {
    Status status = i_block(node->body);
    if(status.signal == Signal::Continue) continue;
    if(status.signal == Signal::Break) break;
  }

  return Status(0);
}

Interpreter::Status Interpreter::i_if(AST::NodeIf *node) {
  if(i_statement(node->primary.condition).exit == 0) return i_block(node->primary.body);
  return Status(0);
}

Interpreter::Status Interpreter::i_while(AST::NodeWhile *node) {
  while(node->is_until ? !i_statement(node->condition).exit : i_statement(node->condition).exit) {
    Status status = i_block(node->body);
    if(status.signal != Signal::None) return status;
  }
  return Status(0);
}