#include <interpreter/interpreter.h>
#include <parser/word.h>
#include <iofuncs.h>



std::string Interpreter::expand(AST::Word* word) {
  std::string str;

  for (auto& e : word->array()) {
    if (e.is<std::string>()) {
      std::string fragment = e.as<std::string>();
      if (fragment == "~" || fragment.starts_with("~/"))
        fragment = (getenv("HOME") ? getenv("HOME") : "") + fragment.substr(1);

      str += fragment;
    }

    if (e.is<AST::NodeVariable*>()) {
      std::string name = e.as<AST::NodeVariable*>()->name;
      if(scope.has(name)) str += scope.get(name);
      else str += getenv(name.c_str()) ? getenv(name.c_str()) : "";
    }
    if(e.is<AST::NodeSubstitution*>()) str += i_substitution(e.as<AST::NodeSubstitution*>());
    if(e.is<AST::NodeString*>()) {
      AST::NodeString* node = e.as<AST::NodeString*>();

      std::string content;
      for(auto& el : node->content)
        content += expand(el);

      str += content;
    }
    if(e.is<AST::NodeEscaped*>()) str += e.as<AST::NodeEscaped*>()->what;
  }
  
  return str;
}
std::string Interpreter::i_substitution(AST::NodeSubstitution *node) {
  int pipefds[2];
  if(pipe(pipefds) != 0) {} // TODO

  int saved_stdout = dup(STDOUT_FILENO);
  if(dup2(pipefds[1], STDOUT_FILENO) == -1) {} // TODO

  close(pipefds[1]);

  i_block(node->body);

  fflush(stdout);
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);

  std::vector<char> buffer(8192);
  ssize_t n = read(pipefds[0], buffer.data(), buffer.size() - 1);
  if(n < 0) {} // TODO

  buffer[n] = '\0';
  close(pipefds[0]);

  std::string result = buffer.data();
  if(result.ends_with("\n")) result.pop_back();

  return result;
}