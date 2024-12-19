#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "ast.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <vector>

struct DebugInfo {
  DICompileUnit *TheCU;
  DIType *DblTy;
  DIType *IntTy;
  std::vector<DIScope *> LexicalBlocks;

  DIType *get_double_type();
  DIType *get_int_type();
  void emit_location(ExprAST *ast);
};

extern DebugInfo KSDbgInfo;

class DebugInfoInserter {
  struct FunctionDebugInfo {
      DIFile *unit;
      DISubprogram *sp;
  } FDI;

public:
  static void emit_location(ExprAST *ast);

  // General methods
  // Function methods
  void insert_subprogram(unsigned line_no, Function * function);
  void insert_function_parameter(unsigned line_no, Argument &arg, AllocaInst *arg_alloca);
  void reset_scope();

};


DISubroutineType *create_function_type(unsigned args_num, bool is_main = false);

#endif
