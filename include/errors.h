#ifndef SLASH_ERRORS_H
#define SLASH_ERRORS_H

#include <string>
#include <variant>

// lexer will only throw if a \ continuation is at EOF
enum class lexer {
  unexpected_eof
};

enum class syntax {
  unexpected_token,
  unexpected_eof
};

enum class runtime {
  syscall_failed,
  file_access_failed,
  invalid_arguments
};

std::string error_to_str(std::variant<syntax, runtime, lexer> error);

enum class Severity { Suggestion, Warning, Error };

std::string severity_color(Severity sev);

class Diagnostic {
private:
  std::string _message;
  Severity _severity{};
  std::variant<syntax, runtime, lexer> _error;

public:
  Diagnostic(std::string msg);
  Diagnostic(Severity sev, std::string msg);
  Diagnostic(syntax syn, std::string msg);
  Diagnostic(runtime run, std::string msg);
  Diagnostic(lexer lex, std::string msg);

  std::string message();
  void dump();
  Severity severity();
};

#endif
