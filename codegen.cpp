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
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <format>
#include <memory>

Function *get_function(const std::string &name) {
  if (auto *f = TheModule->getFunction(name))
    return f;

  auto f = FunctionProtos.find(name);
  if (f != FunctionProtos.end())
    return f->second->codegen();

  return nullptr;
}

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

  if (Op == "+")
    return Builder->CreateFAdd(L, R, "addtmp");
  if (Op == "-")
    return Builder->CreateFSub(L, R, "subtmp");
  if (Op == "*")
    return Builder->CreateFMul(L, R, "multmp");
  if (Op == "<") {
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  }
  if (Op == ">") {
    L = Builder->CreateFCmpULT(R, L, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  }

  auto *f = get_function(std::string("binary") + Op);
  if (!f)
    return log_error_v(
        std::format("Binary operator `{}` not found", Op).c_str());

  Value *Ops[2] = {L, R};
  return Builder->CreateCall(f, Ops, "binop");
}

Value *UnaryExprAST::codegen() {
  auto *f = get_function(std::format("unary{}", Op));
  if (!f)
    return log_error_v(
        std::format("Unary operator {} does not exist.", Op).c_str());

  auto operand = Operand->codegen();
  if (!operand)
    return nullptr;

  return Builder->CreateCall(f, operand);
}

Value *CallExprAST::codegen() {
  Function *CalleeF = get_function(Callee);
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

  auto &p = *Proto;
  Function *F = get_function(Proto->get_name());

  if (!F) {
    FunctionProtos[p.get_name()] = std::move(Proto);
    if (!(F = p.codegen()))
      return nullptr;
  }

  if (F->arg_size() != p.get_arg_size())
    return (Function *)log_error_v(
        std::format("Can not overwrite function {} which has {} arguments"
                    " with a function which has {} arguments",
                    Proto->get_name(), F->arg_size(), Proto->get_arg_size())
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
    TheFPM->run(*F, *TheFAM);

    if (p.is_binary_op())
      BINOP_PRECEDENCE[p.get_operator_name()] = p.get_binary_precedence();
    return F;
  }
  F->eraseFromParent();
  return nullptr;
}

Value *IfExprAST::codegen() {
  Value *cond_val = Condition->codegen();

  if (!cond_val)
    return nullptr;

  auto *bool_cond = Builder->CreateFCmpONE(
      cond_val, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");

  Function *f = Builder->GetInsertBlock()->getParent();

  auto *then_bb = BasicBlock::Create(*TheContext, "then", f);
  auto *else_bb = BasicBlock::Create(*TheContext, "else");
  auto *fin_bb = BasicBlock::Create(*TheContext, "finish");

  Builder->CreateCondBr(bool_cond, then_bb, else_bb);

  Builder->SetInsertPoint(then_bb);
  Value *then_val = Then->codegen();
  if (!then_val)
    return nullptr;
  Builder->CreateBr(fin_bb);
  auto *then_phi_bb = Builder->GetInsertBlock();

  f->insert(f->end(), else_bb);
  Builder->SetInsertPoint(else_bb);
  Value *else_val = Else->codegen();
  if (!else_val)
    return nullptr;

  Builder->CreateBr(fin_bb);
  auto *else_phi_bb = Builder->GetInsertBlock();

  f->insert(f->end(), fin_bb);
  Builder->SetInsertPoint(fin_bb);
  auto *ret_val =
      Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "iftmp");
  ret_val->addIncoming(then_val, then_phi_bb);
  ret_val->addIncoming(else_val, else_phi_bb);

  return ret_val;
}

Value *ForExpr::codegen() {

  Value *start = Start->codegen();
  if (!start)
    return nullptr;

  auto *f = Builder->GetInsertBlock()->getParent();
  auto *first_bb = Builder->GetInsertBlock();
  auto *loop_bb = BasicBlock::Create(*TheContext, "loop", f);
  auto *end_bb = BasicBlock::Create(*TheContext, "endfor");

  Builder->CreateBr(loop_bb);

  Builder->SetInsertPoint(loop_bb);

  auto *var = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "loopval");
  var->addIncoming(start, first_bb);

  Value *old_val = NamedValues[VarName];
  NamedValues[VarName] = var;

  Value *condition = Condition->codegen();
  if (!condition)
    return nullptr;
  auto *bool_cond = Builder->CreateFCmpONE(
      condition, ConstantFP::get(*TheContext, APFloat(0.0)), "forcond");
  auto *branch = Builder->CreateBr(end_bb);

  // Check the condition even on the first iteration
  auto *then_inst = SplitBlockAndInsertIfThen(bool_cond, branch, false);
  auto *split_bb = then_inst->getParent();
  Builder->SetInsertPoint(split_bb);
  then_inst->eraseFromParent();

  // generate Body
  auto *body = Body->codegen();
  if (!body)
    return nullptr;

  Value *step = Step->codegen();
  if (!step)
    return nullptr;
  var->addIncoming(Builder->CreateFAdd(var, step), split_bb);
  Builder->CreateBr(loop_bb);

  f->insert(f->end(), end_bb);
  Builder->SetInsertPoint(end_bb);
  NamedValues[VarName] = old_val;
  return ConstantFP::get(*TheContext, APFloat(0.0));
}
