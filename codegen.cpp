#include "ast.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <format>
#include <memory>


Value *log_error_v(const char *Str) {
  log_error(Str);
  return nullptr;
}

Value *NumberExprAST::codegen() {
  return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
  Value *V = NamedValues[Name];
  if (!V)
    return log_error_v("Unknown variable name");
  return V;
}

Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  default:
    return log_error_v("invalid binary operator");
  }
}

Value *CallExprAST::codegen() {
  Function *CalleeF = TheModule->getFunction(Callee);
  if (!CalleeF)
    return log_error_v(
        std::format("Unknown function {} referenced", Callee).c_str());

  if (CalleeF->arg_size() != Args.size())
    return log_error_v(
        std::format("Incorrect number of arguments for function {}", Callee)
            .c_str());

  std::vector<Value *> ArgsV;
  for (auto &expr : Args) {
    ArgsV.push_back(expr->codegen());
    if (!ArgsV.back()) // if the last element is nullptr
      return nullptr;
  }
  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen() {
  std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
  FunctionType *FT =
      FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  unsigned idx = 0;
  for (auto &arg : F->args())
    arg.setName(Args[idx++]);

  return F;
}

Function *FunctionAST::codegen() {
  Function *F = TheModule->getFunction(Proto->get_name());

  if (!F)
    F = Proto->codegen();

  if (!F)
    return nullptr;

  if (!F->empty())
    return (Function *)log_error_v(
        std::format("Function {} has already been defined", Proto->get_name())
            .c_str());

  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", F);
  Builder->SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &arg : F->args())
    NamedValues[std::string(arg.getName())] = &arg;
  if (Value *ret_value = Body->codegen()) {
    Builder->CreateRet(ret_value);
    verifyFunction(*F);
    return F;
  }
  F->eraseFromParent();
  return nullptr;
}

