#include <interpreter/interpreter.h>
#include <parser/word.h>

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
  }
  
  return str;
}