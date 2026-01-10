#include <iofuncs.h>
#include <prompt/sead.h>
#include <parser/lexer.h>
#include <parser/parser.h>
#include <interpreter/interpreter.h>
#include <prompt/prompt.h>
#include <parser/word.h>
#include <session.h>
#include <csignal>
#include <global.h>

int main() {
  Session::interactive = true;
  int i_errors = 0;

  signal(SIGINT,  SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN); 
  signal(SIGQUIT, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);

  while(true) {
    Sead sead(Prompt::parse_ps(Prompt::get_ps<1>()));
    std::string input = sead.readln();

    if(input == "exit") break;
    if(input.empty()) continue;

    std::vector<Token> tokens;

    try {
      Lexer lexer(input + "\n");
      //lexer.dump();
      tokens = lexer.tokens();
    } catch(Diagnostic& d) {
      d.dump();
      continue;
    }

    AST::NodeBlock* block = new AST::NodeBlock();

    try {
      Parser parser(tokens);
      parser.parse();
      block = parser.block();

      for(auto& s : parser.diags()) s.dump();
    }
    catch(Diagnostic& d) { d.dump(); continue; }

    Global::interpreter.i_statement(block);

    if(Global::interpreter.error_count() > i_errors) {
      for(int i = i_errors; i < Global::interpreter.diags().size(); i++) {
        Global::interpreter.diags()[i].dump();
      }
      i_errors = Global::interpreter.error_count();
    }
  }

  return 0;
}
