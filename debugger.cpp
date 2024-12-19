#include "debugger.h"
#include "internal.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"

DebugInfo KSDbgInfo;

DIType *DebugInfo::get_double_type() {
  if (DblTy)
    return DblTy;

  DblTy = DBuilder->createBasicType("double", 64, dwarf::DW_ATE_float);
  return DblTy;
}

DIType *DebugInfo::get_int_type() {
  if (IntTy)
    return IntTy;

  IntTy = DBuilder->createBasicType("int", 32, dwarf::DW_ATE_signed);
  return IntTy;
}

void DebugInfo::emit_location(ExprAST *ast) {
  if (!ast)
    return Builder->SetCurrentDebugLocation(
        DebugLoc()); // if ast is null set current location to empty DebugLoc

  DIScope *scope;
  if (LexicalBlocks.empty())
    scope = TheCU;
  else
    scope = LexicalBlocks.back();

  auto location = DILocation::get(scope->getContext(), ast->get_line(),
                                  ast->get_col(), scope);
  Builder->SetCurrentDebugLocation(location);
}

DISubroutineType *create_function_type(unsigned args_num, bool is_main) {
  SmallVector<Metadata *, 8> EltTys;
  if (!is_main) {
    auto *DblTy = KSDbgInfo.get_double_type();

    EltTys.push_back(DblTy);
    for (unsigned i = 0, e = args_num; i != e; ++i)
      EltTys.push_back(DblTy);

    return DBuilder->createSubroutineType(
        DBuilder->getOrCreateTypeArray(EltTys));
  }

  EltTys.push_back(KSDbgInfo.get_int_type());
  return DBuilder->createSubroutineType(DBuilder->getOrCreateTypeArray(EltTys));
}

void DebugInfoInserter::insert_subprogram(unsigned line_no,
                                          Function *function) {

  if (!DEBUG)
    return;
  unsigned scope_line = line_no;
  DISubroutineType *SRTy;
  if (function->getName() == "main")
    SRTy = create_function_type(0, true);
  else
    SRTy = create_function_type(function->arg_size());

  FDI.unit = DBuilder->createFile(KSDbgInfo.TheCU->getFilename(),
                                  KSDbgInfo.TheCU->getDirectory());
  DIScope *f_context = FDI.unit;
  FDI.sp = DBuilder->createFunction(
      f_context, function->getName(), StringRef(), FDI.unit, line_no, SRTy,
      scope_line, DINode::FlagPrototyped, DISubprogram::SPFlagDefinition);

  function->setSubprogram(FDI.sp);
  KSDbgInfo.LexicalBlocks.push_back(FDI.sp);
  KSDbgInfo.emit_location(nullptr);
}

void DebugInfoInserter::insert_function_parameter(unsigned line_no,
                                                  Argument &arg,
                                                  AllocaInst *arg_alloca) {

  if (!DEBUG)
    return;

  static unsigned arg_idx = 0;
  DILocalVariable *debug_descriptor = DBuilder->createParameterVariable(
      FDI.sp, arg.getName(), ++arg_idx, FDI.unit, line_no,
      KSDbgInfo.get_double_type(), true);

  DBuilder->insertDeclare(
      arg_alloca, debug_descriptor, DBuilder->createExpression(),
      DILocation::get(FDI.sp->getContext(), line_no, 0, FDI.sp),
      Builder->GetInsertBlock());
}

void DebugInfoInserter::reset_scope() {
  if (DEBUG)
    KSDbgInfo.LexicalBlocks.pop_back();
}

void DebugInfoInserter::emit_location(ExprAST *ast) {
  if (DEBUG)
    KSDbgInfo.emit_location(ast);
}
