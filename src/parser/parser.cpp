#include <parser/parser.h>
#include <parser/nodes.h>
#include <parser/parser.h>
#include <parser/word.h>
#include <prompt/sead.h>
#include <unordered_map>
#include <session.h>
#include <functional>
#include <iofuncs.h>

Token Parser::consume(TokType type, std::string lexeme, bool ws_matters) {
  Token next = peek(1, ws_matters);
  if(next.type != type) {
    if(!lexeme.empty() && next.lexeme != lexeme)
      error(syntax::unexpected_token,
      "unexpected " + toktype_to_str(next.type) + " '" + next.lexeme + "', " + 
      "expected " + toktype_to_str(type) + " '" + lexeme + "'");
  }

  advance();
  return next;
}

std::vector<Token> Parser::until(DepthInfo info) {
  int depth = 0;
  std::vector<Token> result;

  io::debug("started until(info)");

  while(true) {
    if(at_eof()) handle_eof();

    Token cur = current();
    for(auto& i : info.increasers) {
      if(cur == i) { depth++; break; }
    }
    for(auto& d : info.decreasers) {
      if(cur == d) {
        if(depth == 0) { advance(); return result; }
        depth--;
        break;
      }
    }

    result.push_back(cur);
    advance();
  }

  io::debug("ended until info");
  return result;
}

std::vector<Token> Parser::until(std::function<bool(Token)> function) {
  io::debug("started until(func)");
  std::vector<Token> result;
  while(true) {
    if(at_eof()) handle_eof();
    if(function(current())) break;

    result.emplace_back(current());
    advance();
  }

  io::debug("stopped until(func)");
  return result;
}

std::vector<Token> Parser::until(TokType type, std::string lexeme) {
  io::debug("started until(normal)");
    std::vector<Token> result;

    while (true) {
        if (at_eof()) handle_eof();

        Token t = current();
        if (t.type == type && (lexeme.empty() || t.lexeme == lexeme)) {
            break;
        }

        result.emplace_back(t);
        advance();
    }

    io::debug("ended until(normal)");
    return result;
}
Token Parser::peek(int n, bool ws_matters)
{
  int save = i;
  if (!ws_matters) {
    while (current().type == TokType::WORD_SEPARATOR) i++;
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
  if(i - 1 < 0) { return Token(); }

  return tokens[i - 1];
}
void Parser::advance() {
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
    push(statement_parser());
  }
}

AST::NodeBlock *Parser::block() {
  return result;
}

/// Node parsing


AST::Node* Parser::after_parser(AST::Node *node, bool expects_arguments) {
  static std::unordered_map<std::string, std::function<void(std::string)>> redirection = {
    {">", [&](std::string n){
      AST::RedirectionInfo::Redirection r;
      r.file_descriptor = n;
      r.filename = word_parser();
      node->redirection.redirections.emplace_back(r);
    }},
    {">>", [&](std::string n){
      AST::RedirectionInfo::Redirection r;
      r.file_descriptor = n;
      r.filename = word_parser();
      r.append = true;
      node->redirection.redirections.emplace_back(r);
    }},
    {">|", [&](std::string n){
      AST::RedirectionInfo::Redirection r;
      r.file_descriptor = n;
      r.filename = word_parser();
      r.force = true;
      node->redirection.redirections.emplace_back(r);
    }},
    {"<", [&](std::string n) {
      AST::RedirectionInfo::InputRedirection input;
      input.filename = word_parser();
      node->redirection.input = input;
    }}
  };

  while(true) {
    if(at_eof()) break;

    if(current().type == TokType::REDIRECTOR || (current().type == TokType::INTEGER && peek().type == TokType::REDIRECTOR)) {
      std::string n           = current().type == TokType::REDIRECTOR ?        "1"       : current().lexeme;
      std::string redirector  = current().type == TokType::REDIRECTOR ? current().lexeme : peek().lexeme;

      if(redirection.find(redirector) != redirection.end()) {
        if(current().type == TokType::INTEGER) advance();
        advance();

        redirection[redirector](n);
        continue;
      }
    }

    if(current().type == TokType::EXECUTION_FLAG) {
      std::string f = current().lexeme;
      if(f == "@e") node->execution.exit        = true;
      if(f == "@s") node->execution.no_subshell = true;
      if(f == "@v") node->execution.no_env      = true;
      if(f == "@E") node->execution.stderr      = true;
      if(f == "@O") node->execution.stdout      = true;
      if(f == "@t") node->execution.time        = true;
      if(f == "@r") node->execution.repeat      = true;

      advance();
      continue;
    }
    if(current().type == TokType::STATEMENT_SEPARATOR) {
      advance();
      break;
    }
    if(current().type == TokType::WORD_SEPARATOR) {
      advance();
      continue;
    }

    if(current().type == TokType::OPERATOR) {
      std::string l = current().lexeme;

      if(l == "&&" || l == "||" || l == "|" || l == "|&") {
        AST::NodeConnector* connector = new AST::NodeConnector();

        connector->connector = l;
        advance();
        connector->left = node;
        connector->right = statement_parser();

        return connector;
      }
    }

    if(expects_arguments) node->arguments.emplace_back(word_parser());
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
AST::NodeBlock *Parser::block_parser(std::vector<Token> tokens) {
  Parser parser(tokens);
  parser.parse();
  for(auto& e : parser.diags()) diagnostics.push_back(e);

  return parser.block();
}
AST::NodeLoop *Parser::loop_parser()
{
  AST::NodeLoop* node = new AST::NodeLoop();
  advance();
  
  node->body = new AST::NodeBlock();

  if(peek().check(TokType::PUNCTUATION, "(")) { node->body->subshell = true; advance(); }
  else consume(TokType::PUNCTUATION, "{");

  node->body->statements = 
    block_parser(until(DepthInfo(node->body->subshell ? DepthInfo::Parentheses : DepthInfo::Brackets)))->statements;

  return node;
}

AST::NodeLoopControl *Parser::control_parser() {
  AST::NodeLoopControl* node = new AST::NodeLoopControl();
  node->is_break = current().check(TokType::KEYWORD, "break");

  if(peek().type == TokType::INTEGER) { node->n = peek().lexeme; advance(); }
  return node;
}

AST::NodeIf *Parser::if_parser() {
  AST::NodeIf* node = new AST::NodeIf();
  advance();

  node->primary.condition = block_parser(until([&](Token token) {
    return token.check(TokType::PUNCTUATION, "{") || token.check(TokType::PUNCTUATION, "(") ||
           token.check(TokType::KEYWORD, "then");
  }));
  node->primary.body = new AST::NodeBlock();
  node->primary.body->subshell = current().lexeme == "(";
  node->primary.body->uses_sh_style = current().lexeme == "then";

  if(current().lexeme == "then") {
    DepthInfo info;
    info.decreasers = {Token(TokType::KEYWORD, "fi")};
    info.increasers = {Token(TokType::KEYWORD, "then")};

    node->primary.body->statements = block_parser(until(info))->statements;
  } else {
    node->primary.body->statements = 
      block_parser(until(DepthInfo(node->primary.body->subshell ? DepthInfo::Parentheses : DepthInfo::Brackets)))->statements;
  }

  advance();
  return node;
}

AST::NodeWhile* Parser::while_until_parser() {
  AST::NodeWhile* node = new AST::NodeWhile();
  node->is_until = current().lexeme == "until";

  advance();

  node->condition =
    block_parser(until([&](Token token) { return token.lexeme == "{" || token.lexeme == "(" || token.lexeme == "do"; }));

  node->body = new AST::NodeBlock();
  node->body->subshell = current().lexeme == "(";  
  node->uses_sh_style = current().lexeme == "do";
  
  if(node->uses_sh_style) {
    DepthInfo info;
    info.increasers = {Token(TokType::KEYWORD, "do")};
    info.decreasers = {Token(TokType::KEYWORD, "done")};

    node->body->statements = block_parser(until(info))->statements;
  }
  else {
    DepthInfo info(node->body->subshell ? DepthInfo::Parentheses : DepthInfo::Brackets);
    node->body->statements = block_parser(until(info))->statements;
  }

  advance();
  return node;
}

AST::Node *Parser::statement_parser()
{
  static std::unordered_map<std::string, std::function<AST::Node*()>> map = {
    {"exit",     [this]() { return exit_parser(); }},
    {"loop",     [this]() { return loop_parser(); }},
    {"break",    [this]() { return control_parser(); }},
    {"continue", [this]() { return control_parser(); }},
    {"if",       [this]() { return if_parser(); }},
    {"while",    [this]() { return while_until_parser(); }},
    {"until",    [this]() { return while_until_parser(); }},
    {"{",        [this]() { return block_subshell_parser(); }},
    {"(",        [this]() { return block_subshell_parser(); }},
  };

  while(true) {
    Token t = current();
    if(t.type == TokType::STATEMENT_SEPARATOR || t.type == TokType::WORD_SEPARATOR) { advance(); continue; }
    if(at_eof()) handle_eof();

    if(map.find(t.lexeme) != map.end()) {
       return after_parser(map[t.lexeme]());
    } else return after_parser(command_parser(), true);

    break;
  }
}
AST::NodeBlock *Parser::block_subshell_parser() {
  AST::NodeBlock* node = new AST::NodeBlock();
  node->subshell = current().check(TokType::PUNCTUATION, "(");
  advance();

  node->statements = block_parser(until(DepthInfo(node->subshell ? DepthInfo::Parentheses : DepthInfo::Brackets)))->statements;
  return node;
}