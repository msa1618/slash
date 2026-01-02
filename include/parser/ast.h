#ifndef SLASH_AST_H
#define SLASH_AST_H

#include <string>
#include <vector>

namespace AST {
  class Word;
  struct NodeVisitor;

  struct ExecutionFlags {
    bool silent, stdout, stderr, time, no_subshell,
         no_env, exit, repeat;
  };
  struct RedirectionInfo {
    struct Redirection {
      Word* filename;
      std::string file_descriptor;
      
      bool append, force;
    };
    struct InputRedirection { Word* filename; };
    struct HereString { Word* content; };
    struct HereDocument { std::vector<Word*> document; };

    std::vector<Redirection> redirections;

    InputRedirection input;
    HereString herestr;
    HereDocument heredoc;
  };

  struct Node {
    ExecutionFlags execution;
    RedirectionInfo redirection;
    std::vector<Word*> arguments;

    virtual ~Node() = default;

    template <typename T> 
    bool is() const { 
        return dynamic_cast<const T*>(this) != nullptr; 
    }

    template <typename T> 
    T* as() { 
        return dynamic_cast<T*>(this); 
    }

    template <typename T> 
    const T* as() const { 
        return dynamic_cast<const T*>(this); 
    }};

  struct NodeBlock : Node { std::vector<Node*> statements; bool subshell; };
}

#endif