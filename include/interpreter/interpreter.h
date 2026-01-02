#ifndef SLASH_INTERPRETER_H
#define SLASH_INTERPRETER_H

#include <string>
#include <vector>
#include <parser/parser.h>
#include <parser/nodes.h>
#include <unordered_map>

class Scope {
  private:
    std::unordered_map<std::string, std::string> variables;

  public:
    bool has(std::string variable) { return variables.find(variable) != variables.end(); }
    std::string get(std::string variable) { return variables[variable]; }
    void set(std::string variable, std::string value) { variables[variable] = value; }
};

class Interpreter {
  private:
    AST::NodeBlock* block;
    Scope scope;

    int count = 0;
    std::vector<Diagnostic> diagnostics;

    struct Status {
      int exit = 0;

      Status(int exit) : exit(exit) {}
    };

    std::string expand(AST::Word* word);
    Status i_command(AST::NodeCommand* node);
    Status i_exit(AST::NodeExit* node);

    int to_int(std::string str);

  public:
    Interpreter(AST::NodeBlock* block) : block(block) {}
    Interpreter() {}

    void execute(AST::Node* node);
    void error(runtime run, std::string msg);
    int error_count();
    decltype(diagnostics) diags();
};

#endif