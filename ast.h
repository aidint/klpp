#ifndef AST_H
#define AST_H

#include "include/Kaleidoscope.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;
using namespace llvm::orc;

Value *log_error_v(const char *Str);

class ExprAST {
public:
  virtual ~ExprAST();
  virtual Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val);
  Value *codegen() override;
};

class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name);
  Value *codegen() override;
};

class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS);
  Value *codegen() override;
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args);
  Value *codegen() override;
};

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args);
  const std::string &get_name();
  Function *codegen();
};

class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body);
  Function *codegen();
};

class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Condition, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Condition, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
      : Condition(std::move(Condition)), Then(std::move(Then)),
        Else(std::move(Else)) {}

  Value *codegen() override;
};

class ForExpr : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, Condition, Step, Body;

public:
  ForExpr(std::string VariableName, std::unique_ptr<ExprAST> Start,
          std::unique_ptr<ExprAST> Condition, std::unique_ptr<ExprAST> Step,
          std::unique_ptr<ExprAST> Body)
      : VarName(VariableName), Start(std::move(Start)),
        Condition(std::move(Condition)), Step(std::move(Step)),
        Body(std::move(Body)) {}

  Value *codegen() override;
};

std::unique_ptr<ExprAST> log_error(const char *Str);
extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unique_ptr<Module> TheModule;
extern std::map<std::string, Value *> NamedValues;
extern std::unique_ptr<KaleidoscopeJIT> TheJIT;
extern std::unique_ptr<FunctionPassManager> TheFPM;
extern std::unique_ptr<LoopAnalysisManager> TheLAM;
extern std::unique_ptr<FunctionAnalysisManager> TheFAM;
extern std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
extern std::unique_ptr<ModuleAnalysisManager> TheMAM;
extern std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
extern std::unique_ptr<StandardInstrumentations> TheSI;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
extern ExitOnError ExitOnErr;

#endif
