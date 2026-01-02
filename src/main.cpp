#include <iofuncs.h>
#include <prompt/sead.h>
#include <parser/lexer.h>
#include <parser/parser.h>
#include <interpreter/interpreter.h>
#include <parser/word.h>
#include <session.h>

int main() {
  Interpreter interpreter;
  Session::interactive = true;
  int i_errors = 0;

  while(true) {
    Sead sead(geteuid() == 0 ? "# " : "$ ");
    std::string input = sead.readln();

    if(input == "exit") break;
    if(input.empty()) continue;

    Lexer lexer(input + "\n");
    lexer.dump();

    AST::NodeBlock* block = new AST::NodeBlock();

    try {
      Parser parser(lexer);
      parser.parse();
      block = parser.block();

      for(auto& s : parser.diags()) s.dump();
    }
    catch(Diagnostic& d) { d.dump(); continue; }
    
    interpreter.execute(block);
    if(interpreter.error_count() > i_errors) {
      i_errors = interpreter.error_count();
      for(int i = i_errors; i < interpreter.diags().size(); i++) {
        interpreter.diags()[i].dump();
      }
    }
  }

  return 0;
}