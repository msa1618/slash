#include <errors.h>
#include <sstream>
#include <color.h>
#include <iofuncs.h>

std::string severity_color(Severity sev) {
  return sev == Severity::Suggestion
           ? cl::green
           : (sev == Severity::Warning ? cl::yellow : cl::red);
}

Diagnostic::Diagnostic(std::string msg)
  : _message(std::move(msg)) {}

Diagnostic::Diagnostic(Severity sev, std::string msg)
  : _message(std::move(msg)), _severity(sev) {}

Diagnostic::Diagnostic(syntax syn, std::string msg)
  : _message(std::move(msg)), _severity(Severity::Error), _error(syn) {}

Diagnostic::Diagnostic(runtime run, std::string msg)
  : _message(msg), _severity(Severity::Error), _error(run) {}

std::string Diagnostic::message() {
  return _message;
}

void Diagnostic::dump() {
  std::stringstream ss;
  ss << "───\n";
  ss << severity_color(_severity);
  if(_severity == Severity::Error)
    ss << error_to_str(_error);
  else ss << (_severity == Severity::Warning ? "warning" : "suggestion");
  ss << cl::reset;
  ss << ": ";
  ss << message() << "\n";

  io::error(ss.str());
}

Severity Diagnostic::severity() {
  return _severity;
}

std::string error_to_str(std::variant<syntax, runtime> error) {
  return std::visit([](auto e) -> std::string {
    if constexpr (std::is_same_v<decltype(e), syntax>) {
      switch (e) {
        case syntax::unexpected_token: return "syntax::unexpected_token";
        case syntax::unexpected_eof:   return "syntax::unexpected_eof";
      }
    } else if constexpr (std::is_same_v<decltype(e), runtime>) {
      switch (e) {
        case runtime::syscall_failed:     return "runtime::syscall_failed";
        case runtime::file_access_failed: return "runtime::file_access_failed";
        case runtime::invalid_arguments:  return "runtime::invalid_arguments";
      }
    }
    return "unknown_error";
  }, error);
}
