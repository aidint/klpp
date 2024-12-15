#include "internal.h"
#include "lex.h"
#include "parser.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/LegacyPassManager.h"
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
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();

  initialize_module_for_compilation();

  set_lex_source(std::make_unique<std::fstream>("lib/core.kl"));
  handle_unit();

  auto str_stream = std::make_unique<std::stringstream>();
  auto &ss = *str_stream;
  set_lex_source(std::move(str_stream));
  while ((get_unit(ss)) != EOF) {
    handle_unit();
    ss.str("");
    ss.clear();
  }

  auto file_name = "output.o";
  std::error_code EC;
  raw_fd_ostream dest(file_name, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }

  legacy::PassManager pass;
  auto file_type = CodeGenFileType::ObjectFile;

  if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, file_type)){
    errs() << "TheTargetMachine can't emit a file of this type";
    return 1;
  }

  pass.run(*TheModule);
  dest.flush();


  return 0;
}
