#include "parser.h"
#include "ast.h"
#include "lex.h"
#include <map>

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<IRBuilder<>> Builder;
std::unique_ptr<Module> TheModule;
std::map<std::string, Value *> NamedValues;


static int cur_tok;
static int get_next_token() { return cur_tok = gettok(); }

static std::unique_ptr<ExprAST> parse_expression();

std::unique_ptr<PrototypeAST> log_error_p(const char *Str) {
  log_error(Str);
  return nullptr;
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
  default:
    return log_error("unknown token when expecting an expression");
  }
}

// Binary Expression Operations
//
static std::map<char, int> BINOP_PRECEDENCE = {
    {'<', 10},
    {'+', 20},
    {'-', 20},
    {'*', 40},
};

static int get_tok_precedence() {
  if (!isascii(cur_tok))
    return -1;

  // Make sure it's a declared binop.
  if (BINOP_PRECEDENCE.find(cur_tok) == BINOP_PRECEDENCE.end())
    return -1;

  int tok_prec = BINOP_PRECEDENCE[cur_tok];
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
    int binop = cur_tok;
    get_next_token(); // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = parse_primary();
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
  auto LHS = parse_primary();
  if (!LHS)
    return nullptr;

  return parse_binop_rhs(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> parse_prototype() {
  if (cur_tok != tok_identifier)
    return log_error_p("Expected function name in prototype");

  std::string fn_name = identifier_str;
  get_next_token(); // expect '('

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

  return std::make_unique<PrototypeAST>(fn_name, std::move(arg_names));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> parse_definition() {
  get_next_token(); // eat def.
  auto proto = parse_prototype();
  if (!proto)
    return nullptr;

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
    auto proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
  }
  return nullptr;
}

// Top level parsing
//
//
static void initialize_module() {
  // Open a new context and module.

  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("my cool jit", *TheContext);

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

static void handle_definition() {
  if (auto func = parse_definition()) {
    if (auto *IR = func->codegen()) {
      fprintf(stderr, "Read function definition:\n");
      IR->print(errs());
    }
  } else {
    // Skip token for error recovery.
    get_next_token();
  }
}

static void handle_extern() {
  if (auto ext = parse_extern()) {
    if (auto *extIR = ext->codegen()) {
      fprintf(stderr, "Read a function declaration:\n");
      extIR->print(errs());
    }
  } else {
    // Skip token for error recovery.
    get_next_token();
  }
}

static void handle_top_level_expression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto expr = parse_top_level_expression()) {
    if (auto *exprIR = expr->codegen()) {
      fprintf(stderr, "Read top level expression:\n");
      exprIR->print(errs());
    }
  } else {
    // Skip token for error recovery.
    get_next_token();
  }
}
/// top ::= definition | external | expression | ';'
static void main_loop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (cur_tok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      get_next_token();
      break;
    case tok_def:
      handle_definition();
      break;
    case tok_extern:
      handle_extern();
      break;
    default:
      handle_top_level_expression();
      break;
    }
  }
}

int main() {
  // Prime the first token.
  fprintf(stderr, "ready> ");
  get_next_token();

  initialize_module(); 

  // Run the main "interpreter loop" now.
  main_loop();

  // Print out all of the generated code.
  TheModule->print(errs(), nullptr);

  return 0;
}
