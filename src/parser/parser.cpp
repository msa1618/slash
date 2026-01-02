#include <parser/parser.h>
#include <parser/nodes.h>
#include <parser/parser.h>
#include <prompt/sead.h>
#include <session.h>

Token Parser::consume(TokType type, std::string lexeme, bool ws_matters) {
  Token next = peek(ws_matters);
  if(next.type != type) {
    if(!lexeme.empty() && next.lexeme != lexeme)
      error(syntax::unexpected_token,
      "unexpected " + toktype_to_str(next.type) + " '" + next.lexeme + "', " + 
      "expected " + toktype_to_str(type) + " '" + lexeme + "'");
  }

  advance(ws_matters);
  return next;
}

Token Parser::peek(int n, bool ws_matters) {
  int save = i;
  while(current().type == TokType::WORD_SEPARATOR) {
    i++;
  }
  i += n;

  if(at_eof()) handle_eof();

  Token tok = tokens[i];
  i = save;
  return tok;
}

Token& Parser::current(bool ws_matters) {
  return tokens[i];
}

Token Parser::previous(bool ws_matters) {
  if(i - 1 < 0) { throw "not possible pls report if thrown"; }

  return tokens[i - 1];
}
void Parser::advance(bool ws_matters) {
  if(!ws_matters) while(current().type == TokType::WORD_SEPARATOR) {
    i++;
  }
  i++;
}

decltype(Parser::diagnostics) Parser::diags() {
  return diagnostics;
}
bool Parser::clean() { 
  for(auto& e : diagnostics) {
    if(e.severity() == Severity::Error) return false;
  }
  return true;
}
bool Parser::at_eof() {
  return i >= tokens.size();
}

bool Parser::handle_eof() {
    error(syntax::unexpected_eof, "unexpected end of file while parsing");
    return true;

  //Lexer lexer(Sead("> ").readln());
  //for(auto& t : lexer.tokens()) tokens.emplace_back(t);
  //return false;
}

void Parser::error(syntax syn, std::string message) {
  throw Diagnostic(syn, message);
}
void Parser::recover() {
  while(!at_eof()) {
    if(current().type == TokType::STATEMENT_SEPARATOR) {
      advance();
      break;
    }
    advance();
  }
}
void Parser::push(AST::Node *node) {
  result->statements.emplace_back(node);
}
void Parser::parse() {
  while(!at_eof()) {
    Token t = current();
    if(t.type == TokType::STATEMENT_SEPARATOR || t.type == TokType::WORD_SEPARATOR) advance();
    if(at_eof()) break;

    if(current().check(TokType::KEYWORD, "exit")) push(exit_parser());
    else push(command_parser());
  }
}

AST::NodeBlock *Parser::block() {
  return result;
}

/// Node parsing


AST::Node* Parser::after_parser(AST::Node *node, bool expects_arguments) {
  while(true) {
    if(at_eof()) break;

    if(current().type == TokType::EXECUTION_FLAG) {
      std::string f = current().lexeme;
      if(f == "@e") node->execution.exit = true;
      if(f == "@s") node->execution.no_subshell = true;
      if(f == "@v") node->execution.no_env = true;
      if(f == "@E") node->execution.stderr = true;
      if(f == "@O") node->execution.stdout = true;
      if(f == "@t") node->execution.time = true;
      if(f == "@r") node->execution.repeat = true;

      advance();
      continue;
    }
    if(current().type == TokType::STATEMENT_SEPARATOR) {
      advance();
      break;
    }

    if(expects_arguments) node->arguments.push_back(word_parser());
    else {
      error(syntax::unexpected_token, "current statement does not expect arguments");
      recover();
      break;
    }
  }
  return node;
}

AST::NodeCommand* Parser::command_parser() {
  AST::NodeCommand* node = new AST::NodeCommand();
  node->program = word_parser();

  return node;
}
AST::NodeExit *Parser::exit_parser() {
  advance();

  AST::NodeExit* node = new AST::NodeExit();
  if(current().type == TokType::STATEMENT_SEPARATOR) node->code = "";
  else node->code = consume(TokType::INTEGER).lexeme;

  return node;
}