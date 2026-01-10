#ifndef SLASH_INTERPRETER_H
#define SLASH_INTERPRETER_H

#include <string>
#include <vector>
#include <optional>
#include <parser/parser.h>
#include <parser/nodes.h>
#include <parser/word.h>
#include <unordered_map>
#include <unistd.h>
#include <session.h>

class Scope {
  private:
    std::unordered_map<std::string, std::string> variables;

  public:
    bool has(std::string variable) { return variables.find(variable) != variables.end(); }
    std::string get(std::string variable) {
      if(variable == "?") return std::to_string(Session::last_exit);
      if(variable == "$") return std::to_string(getpid());

      return variables[variable];
    }
    void set(std::string variable, std::string value) { variables[variable] = value; }
};

class Interpreter {
  private:
    AST::NodeBlock* block;
    Scope scope;

    int count{};
    std::vector<Diagnostic> diagnostics;

    enum class Signal { None, Break, Continue };

    struct Status {
      int exit = 0;
      Signal signal = Signal::None;

      Status(int exit, Signal sig = Signal::None) : exit(exit), signal(sig) {}
    };

    std::string i_substitution(AST::NodeSubstitution* node);

    Status i_block(AST::NodeBlock* node);
    std::pair<bool, std::string> is_script(std::string file);
    

    Status i_command(AST::NodeCommand* node);
    Status i_exit(AST::NodeExit* node);
    Status i_loop(AST::NodeLoop* node);
    Status i_connector(AST::NodeConnector* node);

    Status i_if(AST::NodeIf* node);
    Status i_while(AST::NodeWhile* node);

    Status i_vardecl(AST::NodeVariableDeclaration* node);
    
    Status execute(AST::Node* node);

    // Builtins
    bool is_builtin(std::string cmd);
    Status i_builtin(std::string cmd, int argc, char* argv[]);

    Status i_cd(int argc, char* argv[]);
    Status i_read(int argc, char* argv[]);
    Status i_fc(int argc, char* argv[]);
    Status i_noop();


    int to_int(std::string str);

  public:
    Interpreter(AST::NodeBlock* block) : block(block) {}
    Interpreter() {}

    Status i_statement(AST::Node* node);
    std::string expand(AST::Word* word);
    Scope get_scope();

    void error(runtime run, std::string msg);
    int error_count();
    decltype(diagnostics) diags();
};

#endif