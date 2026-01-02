#include <interpreter/interpreter.h>
#include <iofuncs.h>

void Interpreter::execute(AST::Node *node) {
  if(node->is<AST::NodeBlock>()) {
    for(auto& n : node->as<AST::NodeBlock>()->statements) execute(n);
  }

  if(node->is<AST::NodeExit>())    i_exit(node->as<AST::NodeExit>());
  if(node->is<AST::NodeCommand>()) {
    i_command(node->as<AST::NodeCommand>());
  }
}
void Interpreter::error(runtime run, std::string msg) {
  count++;
  diagnostics.push_back(Diagnostic(run, msg));
}
int Interpreter::error_count() {
  return count;
}
int Interpreter::to_int(std::string str) {
  if(str.starts_with("0x") || str.starts_with("0X")) return std::stoi(str.substr(2), nullptr, 16);
  if(str.starts_with("0b") || str.starts_with("0B")) return std::stoi(str.substr(2), nullptr, 2);
  if(str.starts_with("0")) return std::stoi(str.substr(2), nullptr, 8);
  return std::stoi(str, nullptr, 10);
}

decltype(Interpreter::diagnostics) Interpreter::diags() {
  return diagnostics;
}
