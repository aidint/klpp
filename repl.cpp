#include "llvm/Support/TargetSelect.h"
#include "internal.h"
#include "include/Kaleidoscope.h"
#include <fstream>
#include <ios>
#include <iterator>
#include "parser.h"

using namespace llvm;

/// top ::= definition | external | expression | ';'
static void main_loop() {
  get_next_token();
  while (true) {
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
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  TheJIT = ExitOnErr(KaleidoscopeJIT::Create());
  initialize_modules_and_managers();

  std::ifstream standard_lib("lib/std.kl");
  lex_iterator = std::istream_iterator<char>(standard_lib >> std::noskipws);

  fprintf(stderr, REPL_STR);
  main_loop(); // load standard library

  return 0;
}
