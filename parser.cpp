#include "parser.h"
#include "ast.h"
#include "internal.h"
#include "lex.h"
#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <string>

// The main code

int cur_tok = 0;
static std::unique_ptr<ExprAST> parse_expression();

void delete_function_if_exists(const std::string &name) {
  auto rt = FunctionRTs.find(name);
  if (rt != FunctionRTs.end()) {
    ExitOnErr(rt->second->get()->remove());
    FunctionRTs.erase(rt);
  }
}

/// numberexpr ::= number
static std::unique_ptr<ExprAST> parse_number_expr() {
  auto result = std::make_unique<NumberExprAST>(num_val);
  get_next_token(); // consume the number
  return std::move(result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> parse_paren_expr() {
  get_next_token(); // cur_tok will become the token after '('
  auto V = parse_expression();
  if (!V)
    return nullptr;

  if (cur_tok != ')')
    return log_error("expected ')'");
  get_next_token(); // get the token after ')'
  return V;
}

// ifexpr ::= 'if' 'then' 'else'
static std::unique_ptr<ExprAST> parse_if_expr() {
  get_next_token(); // eat if;

  auto cond = parse_expression();
  if (!cond)
    return nullptr;

  if (cur_tok != tok_then)
    return log_error("expected `then`");
  get_next_token(); // eat then

  auto then = parse_expression();
  if (!then)
    return nullptr;

  std::unique_ptr<ExprAST> else_;
  if (cur_tok == tok_else) {
    get_next_token(); // eat else
    else_ = parse_expression();
    if (!else_)
      return nullptr;
  } else
    else_ = std::make_unique<NumberExprAST>(0);

  return std::make_unique<IfExprAST>(std::move(cond), std::move(then),
                                     std::move(else_));
}

static std::unique_ptr<ExprAST> parse_for_expr() {

  get_next_token(); // eat for

  if (cur_tok != tok_identifier)
    return log_error("Expected identifier after `for`");

  auto var = identifier_str;
  get_next_token(); // eat identifier

  if (cur_tok != tok_operator && operator_name != "=")
    return log_error("Expected `=` after identifier for initialization.");

  get_next_token(); // eat =

  auto start = parse_expression();
  if (!start)
    return nullptr;

  if (cur_tok != ',')
    return log_error(
        "Missing ',' separtor in for statement after initialization.");

  get_next_token(); // eat ,

  auto condition = parse_expression();
  if (!condition)
    return nullptr;

  if (cur_tok != ',')
    return log_error("Missing ',' separtor in for statement after condition.");

  get_next_token(); // eat ,

  auto step = parse_expression();
  if (!step)
    return nullptr;

  if (cur_tok != tok_do)
    return log_error("Expected `do` in for statement.");

  get_next_token(); // eat do

  auto body = parse_expression();
  if (!body)
    return nullptr;

  if (cur_tok != tok_end)
    return log_error("Missing `end`.");

  get_next_token(); // eat end

  return std::make_unique<ForExprAST>(var, std::move(start),
                                      std::move(condition), std::move(step),
                                      std::move(body));
}

static std::unique_ptr<ExprAST> parse_with_expr() {

  get_next_token(); // eat with

  VariableVector Variables;

  do {
    if (cur_tok != tok_identifier)
      return log_error("With statement expects valid identifier.");
    auto variable_name = identifier_str;
    get_next_token(); // eat identifier

    std::unique_ptr<ExprAST> initial_val;
    if (cur_tok == tok_operator && operator_name == "=") {
      get_next_token(); // eat =
      initial_val = parse_expression();
      if (!initial_val)
        return nullptr;
    }

    Variables.push_back(std::make_pair(variable_name, std::move(initial_val)));

    if (cur_tok != ',')
      break;
    get_next_token(); // eat ,

  } while (true);

  if (cur_tok != tok_do)
    return log_error("Missing `do` keyword.");

  get_next_token(); // eat do

  auto body = parse_expression();
  if (!body)
    return nullptr;

  if (cur_tok != tok_end)
    return log_error("Missing `end` keyword.");
  get_next_token(); // eat end

  return std::make_unique<WithExprAST>(std::move(Variables), std::move(body));
}

/// identifierexpr
///   ::= identifier  // simple variable ref
///   ::= identifier '(' expression* ')' // function call
static std::unique_ptr<ExprAST> parse_identifier_expr() {
  std::string id_name = identifier_str;

  get_next_token(); // eat identifier.

  if (cur_tok != '(') // Simple variable ref.
    return std::make_unique<VariableExprAST>(id_name);

  // Call.
  get_next_token(); // eat (
  std::vector<std::unique_ptr<ExprAST>> args;
  if (cur_tok != ')') {
    while (true) {
      if (auto arg = parse_expression())
        args.push_back(std::move(arg));
      else
        return nullptr;

      if (cur_tok == ')')
        break;

      if (cur_tok != ',')
        return log_error("Expected ')' or ',' in argument list");
      get_next_token(); // eat ,
    }
  }

  // Eat the ')'.
  get_next_token();

  return std::make_unique<CallExprAST>(id_name, std::move(args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> parse_primary() {
  switch (cur_tok) {
  case tok_identifier:
    return parse_identifier_expr();
  case tok_number:
    return parse_number_expr();
  case '(':
    return parse_paren_expr();
  case tok_if:
    return parse_if_expr();
  case tok_for:
    return parse_for_expr();
  case tok_with:
    return parse_with_expr();
  default:
    return log_error("unknown token when expecting an expression");
  }
}

/// unary
///   ::= primary
///   ::= <unary operator>unary
static std::unique_ptr<ExprAST> parse_unary() {
  if (cur_tok != tok_operator)
    return parse_primary();

  std::string op = operator_name;
  get_next_token();

  // although in this implementation we don't assume operators as single
  // characters but one can chain these operators by putting a space in between
  // so:
  //  !!s == unary!!(s)
  //  while
  //  ! ! s == unary!(unary!(s))
  if (auto operand = parse_unary())
    return std::make_unique<UnaryExprAST>(op, std::move(operand));

  return nullptr;
}

static int get_tok_precedence() {
  if (cur_tok != tok_operator)
    return -1;

  // Make sure it's a declared binop.
  if (BINOP_PRECEDENCE.find(operator_name) == BINOP_PRECEDENCE.end())
    return -1;

  int tok_prec = BINOP_PRECEDENCE[operator_name];
  if (tok_prec <= 0)
    return -1;
  return tok_prec;
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> parse_binop_rhs(int expr_prec,
                                                std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    // if current token is not a binop, then tok_prec will be -1
    // and since we call parse_binop_rhs with expr_prec > 0, it will return LHS
    int tok_prec = get_tok_precedence();

    // This means that in the case of a * b + c, when it comes to '+',
    // it will stop and return the LHS which will be <<a> * <b>>
    if (tok_prec < expr_prec)
      return LHS;
    // Okay, we know this is a binop.
    std::string binop = operator_name;
    get_next_token(); // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = parse_unary();
    if (!RHS)
      return nullptr;
    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int next_prec = get_tok_precedence();
    if (tok_prec < next_prec) {
      // tok_prec + 1 because anything with the current precedence should not be
      // parsed in the LHS
      RHS = parse_binop_rhs(tok_prec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }
    LHS =
        std::make_unique<BinaryExprAST>(binop, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= primary binoprhs
///

static std::unique_ptr<ExprAST> parse_expression() {
  auto LHS = parse_unary();
  if (!LHS)
    return nullptr;

  return parse_binop_rhs(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> parse_prototype() {

  std::string fn_name;
  unsigned char kind = 0;   // 0 = identifier, 1 = unary, 2 = binary
  unsigned precedence = 30; // default precedence

  switch (cur_tok) {
  default:
    return log_error_p("Expected function name in prototype");
  case tok_identifier:
    fn_name = identifier_str;
    get_next_token(); // expect '('
    break;
  case tok_binary:
    fn_name = "binary" + operator_name;
    kind = 2;
    get_next_token(); // expect '(' or number

    if (cur_tok == tok_number) {
      if (num_val < 1 || num_val > 100)
        return log_error_p("Invalid precedence: must be 1..100");
      precedence = num_val;
      get_next_token(); // expect '('
    }
    break;
  case tok_unary:
    fn_name = "unary" + operator_name;
    kind = 1;
    get_next_token(); // expect '('
    break;
  }

  if (cur_tok != '(')
    return log_error_p("Expected '(' in prototype");

  // Read the list of argument names.
  std::vector<std::string> arg_names;

  // I'm going to change this to expect ',' as argument separator
  while (get_next_token() == tok_identifier)
    arg_names.push_back(identifier_str);

  if (cur_tok != ')')
    return log_error_p("Expected ')' in prototype");

  // success.
  get_next_token(); // eat ')'.
  //
  if (kind && arg_names.size() != kind)
    return log_error_p("Invalid number of operands for operator.");

  return std::make_unique<PrototypeAST>(fn_name, std::move(arg_names),
                                        kind != 0, precedence);
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> parse_definition() {
  get_next_token(); // eat def.
  auto proto = parse_prototype();
  if (!proto)
    return nullptr;

  delete_function_if_exists(proto->get_name());
  if (auto E = parse_expression())
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
  return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> parse_extern() {
  get_next_token(); // eat extern.
  return parse_prototype();
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> parse_top_level_expression() {
  if (auto E = parse_expression()) {
    // Make an anonymous proto.
    auto proto = std::make_unique<PrototypeAST>(ANON_FUNCTION,
                                                std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
  }
  return nullptr;
}

// Top level parsing
//
//

void handle_definition() {
  if (auto func = parse_definition()) {
    std::string function_name = func->get_name();
    if (auto *IR = func->codegen()) {
      if (DEBUG) {
        fprintf(stderr, "Read function definition:\n");
        IR->print(errs());
        fprintf(stderr, "\n" REPL_STR);
      }

      auto RT = std::make_unique<ResourceTrackerSP>(
          TheJIT->getMainJITDylib().createResourceTracker());
      auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
      ExitOnErr(TheJIT->addModule(std::move(TSM), *RT));
      initialize_modules_and_managers();

      FunctionRTs[function_name] = std::move(RT);
    }
  } else {
    // Skip token for error recovery.
    get_next_token();
  }
}

void handle_extern() {
  if (auto ext = parse_extern()) {
    if (auto *extIR = ext->codegen()) {
      if (DEBUG) {
        fprintf(stderr, "Read a function declaration:\n");
        extIR->print(errs());
        fprintf(stderr, "\n" REPL_STR);
      }
      FunctionProtos[ext->get_name()] = std::move(ext);
    }
  } else {
    // Skip token for error recovery.
    get_next_token();
  }
}

void handle_top_level_expression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto expr = parse_top_level_expression()) {
    if (expr->codegen()) {

      auto RT = TheJIT->getMainJITDylib().createResourceTracker();
      auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
      ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
      initialize_modules_and_managers();

      auto expr_symbol = ExitOnErr(TheJIT->lookup(ANON_FUNCTION));

      auto fp = expr_symbol.getAddress().toPtr<double (*)()>();


      fprintf(stderr, DEBUG ? "\r  \tEvaluated to: %lf\n" REPL_STR : "\r  \t%lf\n" REPL_STR,
              fp());
      ExitOnErr(RT->remove());
    }
  } else {
    // Skip token for error recovery.
    get_next_token();
  }
}
