#include "Kaleidoscope.h"
#include "internal.h"
#include "lex.h"
#include "parser.h"
#include "llvm/Support/TargetSelect.h"
#include <fstream>
#include <sstream>

using namespace llvm;

// read one unit of translation
int get_unit(std::stringstream &ss) {
  char c;
  bool in_comment = false;
  while ((c = getchar())) {
    ss << c;
    if (c == '#')
      in_comment = true;
    if (c == '\n') {
      fprintf(stderr, REPL_STR);
      in_comment = false;
    }
    if ((!in_comment && c == ';')) 
      return 0;
    if (c == EOF)
      return EOF;
  }
  return 0;
}

/// top ::= definition | external | expression | ';'
static void handle_unit() {
  get_next_token();
  while (true) {
    switch (cur_tok) {
    case tok_eof:
      return; // should not happen
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
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  TheJIT = ExitOnErr(KaleidoscopeJIT::Create());
  initialize_modules_and_managers_for_jit();

  fprintf(stderr, REPL_STR);

  set_lex_source(std::make_unique<std::fstream>("lib/core.hkl"));
  handle_unit();

  set_lex_source(std::make_unique<std::fstream>("lib/core.kl"));
  handle_unit();

  set_lex_source(std::make_unique<std::fstream>("lib/builtin.kl"));
  handle_unit();

  auto str_stream = std::make_unique<std::stringstream>();
  auto &ss = *str_stream;
  set_lex_source(std::move(str_stream));
  while ((get_unit(ss)) != EOF) {
    handle_unit();
    ss.str("");
    ss.clear();
  }

  return 0;
}
